#include "rip_ipc.h"
#include "utils/event.h"
#include "utils/logging.h"
#include "utils/utils.h"
#include <errno.h>
#include <fcntl.h>
#include <mqueue.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#define RIP_DEAMON_QUEUE "/rip_queue"
#define RIP_CLI_QUEUE "/rip_cli_queue"
#define REQ_BUFFER_SIZE 128
#define QUEUE_PERMISSIONS 0660;

struct rip_ipc {
	mqd_t			  fd;
	struct r_ipc_cmd_handler *cmd_h;
	size_t			  cmd_h_len;
	const char		 *queue_name;
};

static struct r_ipc_cmd_handler *find_handler(const struct rip_ipc *ri, enum rip_ipc_cmd cmd)
{
	for (size_t i = 0; i < ri->cmd_h_len; ++i) {
		struct r_ipc_cmd_handler *hl = &ri->cmd_h[i];
		if (cmd == hl->cmd) {
			return hl;
		}
	}
	return NULL;
}

static struct rip_ipc *rip_ipc_alloc(void) { return CALLOC(sizeof(struct rip_ipc)); }
void		       rip_ipc_free(struct rip_ipc *ri)
{
	if (ri->fd > 0) {
		mq_close(ri->fd);
		mq_unlink(ri->queue_name);
	}
	free(ri);
}

struct rip_ipc *rip_ipc_alloc_init(struct r_ipc_cmd_handler handlers[], size_t len)
{
	struct rip_ipc *ri = NULL;
	mqd_t		fd;
	struct mq_attr	attr = {
	     .mq_curmsgs = 0, .mq_flags = 0, .mq_maxmsg = 10, .mq_msgsize = REQ_BUFFER_SIZE};
	mode_t	    permission = QUEUE_PERMISSIONS;
	const char *q_name     = RIP_DEAMON_QUEUE;

	ri = rip_ipc_alloc();
	if (!ri) {
		return NULL;
	}

	fd = mq_open(q_name, O_RDONLY | O_CREAT | O_NONBLOCK, &permission, &attr);
	if (fd == -1) {
		LOG_ERR("mq_open(%s) failed: %s", q_name, strerror(errno));
		rip_ipc_free(ri);
		return NULL;
	}

	LOG_INFO("IPC initialized, queue name: %s, fd : %d", q_name, fd);
	ri->fd	       = fd;
	ri->cmd_h      = handlers;
	ri->cmd_h_len  = len;
	ri->queue_name = q_name;

	return ri;
}
int rip_ipc_getfd(struct rip_ipc *ri) { return ri->fd; }

int rip_ipc_handle_event(const struct event *event)
{
	int		ret = 0;
	struct rip_ipc *ri  = event->arg;

	ssize_t			  n_bytes;
	mqd_t			  cli_q				  = 0;
	char			  ipc_req_buffer[REQ_BUFFER_SIZE] = {0};
	struct ipc_request	 *req;
	struct r_ipc_cmd_handler *hl;
	struct ipc_response	 *response	= NULL;
	enum r_cmd_status	  status	= r_cmd_status_failed;
	FILE			 *buffer_stream = NULL;

	n_bytes = mq_receive(ri->fd, ipc_req_buffer, REQ_BUFFER_SIZE, NULL);
	RIP_ASSERT(n_bytes >= 0);

	cli_q = mq_open(RIP_CLI_QUEUE, O_WRONLY);
	if (cli_q == -1) {
		LOG_ERR("mq_open(%s) failed: %s", RIP_CLI_QUEUE, strerror(errno));
		ret = 1;
		goto cleanup;
	}

	response = CALLOC(sizeof(struct ipc_response));
	if (!response) {
		LOG_ERR("calloc failed");
		ret = 1;
		goto cleanup;
	}

	req = (struct ipc_request *)ipc_req_buffer;
	hl  = find_handler(ri, req->cmd);
	if (!hl) {
		LOG_ERR("Handler not found, cmd: %d", req->cmd);
		ret = 1;
		goto cleanup;
	}

	buffer_stream = fmemopen(response->output, sizeof(response->output), "wr");

	if (NULL == buffer_stream) {
		LOG_ERR("fmemopen");
	} else {
		status = hl->cb(buffer_stream, hl->data);
		if (ferror(buffer_stream)) {
			LOG_ERR("Error Writing to buffer_stream");
			status = r_cmd_status_failed;
		}

		// needs to be invoked before mq_send to flush the buffer
		fflush(buffer_stream);
	}

	if (status != r_cmd_status_success) {
		LOG_ERR("Failed to execute: %d", hl->cmd);
		ret = 1;
	}

	response->cmd_status = status;
	if (mq_send(cli_q, (const char *)response, sizeof(struct ipc_response), 0) == -1) {
		LOG_ERR("mq_send failed: %s", strerror(errno));
		ret = 1;
	}

cleanup:
	free(response);
	if (buffer_stream) {
		fclose(buffer_stream);
	}
	if (cli_q) {
		mq_close(cli_q);
	}
	return ret;
}

struct rip_ipc *cli_rip_ipc_alloc_init(void)
{
	struct rip_ipc *ri = NULL;
	mqd_t		fd;
	struct mq_attr	attr = {.mq_curmsgs = 0,
				.mq_flags   = 0,
				.mq_maxmsg  = 10,
				.mq_msgsize = sizeof(struct ipc_response)};

	mode_t	    permission = QUEUE_PERMISSIONS;
	const char *q_name     = RIP_CLI_QUEUE;

	ri = rip_ipc_alloc();
	if (!ri) {
		LOG_ERR("rip_ipc_alloc");
		exit(1);
	}

	fd = mq_open(q_name, O_RDONLY | O_CREAT, &permission, &attr);
	if (fd == -1) {
		fprintf(stderr, "mq_open(%s) failed: %s", q_name, strerror(errno));
		exit(1);
	}

	ri->fd	       = fd;
	ri->queue_name = q_name;

	return ri;
}

void cli_rip_ipc_send_msg(struct rip_ipc *ri, struct ipc_request request, struct ipc_response *resp)
{
	mqd_t deamons_fd;

	deamons_fd = mq_open(RIP_DEAMON_QUEUE, O_WRONLY);
	if (deamons_fd < 0) {
		printf("(cli): RIP deamon not up");
		exit(1);
	}

	if (mq_send(deamons_fd, (const char *)&request, sizeof(request), 0) < 0) {
		printf("(cli): mq_send failed: %s", strerror(errno));
		exit(1);
	}
	mq_close(deamons_fd);

	struct timespec timeout;
	clock_gettime(CLOCK_REALTIME, &timeout);
	timeout.tv_sec += 3;
	if (mq_timedreceive(ri->fd, (char *)resp, sizeof(struct ipc_response), NULL, &timeout) <
	    0) {
		printf("(cli): mq_receive failed: %s", strerror(errno));
		exit(1);
	}
	resp->output[sizeof(resp->output) - 1] = '\0';
}
