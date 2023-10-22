#include "rip_ipc.h"
#include "logging.h"
#include "utils.h"
#include <errno.h>
#include <fcntl.h>
#include <mqueue.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define RIP_DEAMON_QUEUE "/rip_queue"
#define RIP_CLI_QUEUE "/rip_cli_queue"
#define REQ_BUFFER_SIZE 128
#define QUEUE_PERMISSIONS 0660;

struct rip_ipc {
	mqd_t fd;
	struct r_ipc_cmd_handler *cmd_h;
	size_t cmd_h_len;
};

static struct r_ipc_cmd_handler *find_handler(const struct rip_ipc *ri,
						enum rip_ipc_cmd cmd)
{
	for (size_t i = 0; i < ri->cmd_h_len; ++i) {
		struct r_ipc_cmd_handler *hl = &ri->cmd_h[i];
		if (cmd == hl->cmd) {
			return hl;
		}
	}
	return NULL;
}

struct rip_ipc *rip_ipc_alloc(void)
{
	return calloc(sizeof(struct rip_ipc), 1);
}
void rip_ipc_free(struct rip_ipc *ri)
{
	if (ri->fd > 0) {
		mq_close(ri->fd);
	}
	free(ri);
}

static mqd_t ipc_open(const char *mq_name, int flags, mode_t *rights,
		      struct mq_attr *attr)
{
	mqd_t fd;

	if (!rights || !attr) {
		fd = mq_open(mq_name, flags);
	} else if (rights && attr) {
		fd = mq_open(mq_name, flags, *rights, attr);
	} else {
		BUG();
	}

	if (fd == -1) {
		LOG_ERR("mq_open(%s) failed: %s", mq_name, strerror(errno));
		return -1;
	}

	return fd;
}

int rip_ipc_init(struct rip_ipc *ri, struct r_ipc_cmd_handler handlers[],
		 size_t len)
{
	mqd_t fd;
	struct mq_attr attr = {.mq_curmsgs = 0,
			       .mq_flags   = 0,
			       .mq_maxmsg  = 10,
			       .mq_msgsize = sizeof(struct ipc_request)};
	mode_t permission   = QUEUE_PERMISSIONS;

	fd = ipc_open(RIP_DEAMON_QUEUE, O_RDONLY | O_CREAT | O_NONBLOCK,
		      &permission, &attr);
	if (fd == -1) {
		return 1;
	}

	ri->fd	      = fd;
	ri->cmd_h     = handlers;
	ri->cmd_h_len = len;

	return 0;
}
int rip_ipc_getfd(struct rip_ipc *ri) { return ri->fd; }

void rip_ipc_handle_msg(struct rip_ipc *ri)
{
	char ipc_req_buffer[REQ_BUFFER_SIZE] = {0};
	ssize_t n_bytes;
	struct ipc_request *req;
	struct r_ipc_cmd_handler *hl;
	mqd_t cli_q;
	struct ipc_response *response = NULL;

	n_bytes = mq_receive(ri->fd, ipc_req_buffer, REQ_BUFFER_SIZE, NULL);
	if (n_bytes < 0) {
		LOG_ERR("mq_receive failed: %s", strerror(errno));
		return;
	}

	req   = (struct ipc_request *)ipc_req_buffer;
	cli_q = ipc_open(RIP_CLI_QUEUE, O_WRONLY, NULL, NULL);
	if (cli_q == -1) {
		return;
	}

	response = calloc(1, sizeof(struct ipc_response));
	if (!response) {
		LOG_ERR("calloc failed");
		goto cleanup;
	}

	hl = find_handler(ri, req->cmd);
	if (!hl) {
		LOG_ERR("Handler not found, cmd: %d", req->cmd);
		goto cleanup;
	}

	int status =
	    hl->cb(response->output, sizeof(response->output), hl->data);
	if (status != 0) {
		LOG_ERR("Failed to execute: %d", hl->cmd);
		response->cmd_status = r_cmd_status_failed;
	}

	response->cmd_status = r_cmd_status_success;
	if (mq_send(cli_q, (const char *)response, sizeof(struct ipc_response),
		    0) == -1) {
		LOG_ERR("mq_send failed: %s", strerror(errno));
	}

cleanup:
	free(response);
	mq_close(cli_q);
}

void rip_ipc_init_cli(struct rip_ipc *ri)
{
	mqd_t fd;
	struct mq_attr attr = {.mq_curmsgs = 0,
			       .mq_flags   = 0,
			       .mq_maxmsg  = 10,
			       .mq_msgsize = sizeof(struct ipc_response)};

	mode_t permission = QUEUE_PERMISSIONS;

	fd = ipc_open(RIP_CLI_QUEUE, O_RDONLY | O_CREAT, &permission, &attr);
	if (fd == -1) {
		exit(1);
	}

	ri->fd = fd;
}

void rip_ipc_send_msg_cli(struct rip_ipc *ri, struct ipc_request request,
			  struct ipc_response *resp)
{
	mqd_t deamons_fd;

	deamons_fd = ipc_open(RIP_DEAMON_QUEUE, O_WRONLY, NULL, NULL);
	if (deamons_fd < 0) {
		LOG_ERR("RIP deamon not up");
		exit(1);
	}

	if (mq_send(deamons_fd, (const char *)&request, sizeof(request), 0) <
	    0) {
		LOG_ERR("ms_send failed: %s", strerror(errno));
		exit(1);
	}
	struct timespec timeout = {
	    .tv_nsec = 0,
	    .tv_sec  = 3,
	};

	if (mq_timedreceive(ri->fd, (char *)resp, sizeof(*resp), NULL,
			    &timeout) < 0) {
		LOG_ERR("mq_receive failed: %s", strerror(errno));
		exit(1);
	}
	resp->output[sizeof(resp->output) - 1] = '\0';
}
