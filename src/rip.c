#include "rip.h"
#include "logging.h"
#include "rip_messages.h"

#include "rip_if.h"
#include "utils.h"
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>

#define RIP_PORT 520
#define MSGBUFSIZE 1024 * 32
#define ANY_IF_INDEX 0
#define POLL_FDS_MAX_SIZE 128

static void rip_if_entry_print(FILE *output, const rip_if_entry *e)
{
	fprintf(output, "fd: %d, if_name: %s, if_index : %d, if_addr: %s\n",
		e->fd, e->if_name, e->if_index, e->if_addr);
}

static int rip_if_entry_setup_resources(rip_if_entry *if_entry)
{
	if (create_udp_socket(&if_entry->fd)) {
		return 1;
	}
	if (set_allow_reuse_addr(if_entry->fd)) {
		return 1;
	}
	if (bind_port(if_entry->fd)) {
		return 1;
	}
	if (join_multicast(if_entry->fd, if_entry->if_index,
			   if_entry->if_addr)) {
		return 1;
	}

	return 0;
}

static int setup_resources(rip_context *rip_ctx)
{
	LOG_INFO("%s", __func__);

	for (size_t i = 0; i < rip_ctx->rip_ifs_count; ++i) {
		rip_if_entry *entry = &rip_ctx->rip_ifs[i];

		if (rip_if_entry_setup_resources(entry)) {
			LOG_ERR("rip_if_entry_setup_resources failed");
			rip_if_entry_print(stderr, entry);
			return 1;
		}
	}

	return 0;
}

static int rip_handle_io(rip_context *rip_ctx, const size_t rip_if_entry_idx)
{
	const rip_if_entry *rip_if_e = &rip_ctx->rip_ifs[rip_if_entry_idx];

	char buff[MSGBUFSIZE];
	struct sockaddr_in addr_n;
	socklen_t addrlen = sizeof(addr_n);
	ssize_t nbytes	  = recvfrom(rip_if_e->fd, buff, MSGBUFSIZE, 0,
				     (struct sockaddr *)&addr_n, &addrlen);
	if (nbytes < 0) {
		LOG_ERR("recvfrom failed: %s", strerror(errno));
		return 1;
	}

	char addr_h[INET_ADDRSTRLEN];
	if (NULL !=
	    inet_ntop(AF_INET, &addr_n.sin_addr, addr_h, INET_ADDRSTRLEN)) {
		LOG_INFO("Got message from %s on %s interface", addr_h,
			 rip_if_e->if_name);
	} else {
		LOG_ERR("got message, but inet_ntop failed: %s",
			strerror(errno));
		return 1;
	}

	char *buff_p = buff;
	rip_header_print((const rip_header *)buff_p);
	nbytes -= sizeof(rip_header);
	buff_p += sizeof(rip_header);

	const size_t entry_count = nbytes / sizeof(rip2_entry);

	for (size_t i = 0; i < entry_count; ++i) {
		printf("[%zu]\n", i);
		rip2_entry_to_host((rip2_entry *)buff_p);
		rip2_entry_print((const rip2_entry *)buff_p);
		buff_p += sizeof(rip2_entry);
	}

	return 0;
}

static void assign_fds_to_pollfds(const rip_context *rip_ctx,
				  struct pollfd pollfds[],
				  const nfds_t pollfds_capacity,
				  nfds_t *actual_pollfds_count)
{
	assert("too small pollfds capacity" &&
	       pollfds_capacity >= rip_ctx->rip_ifs_count);

	for (size_t i = 0; i < rip_ctx->rip_ifs_count; ++i) {
		pollfds[i].fd	  = rip_ctx->rip_ifs[i].fd;
		pollfds[i].events = POLLIN;
	}
	// tbd: more fds like for timers, terminal interaction
	*actual_pollfds_count = rip_ctx->rip_ifs_count;
}

int rip_if_entry_find_by_fd(const rip_context *rip_ctx, const int fd,
			    size_t *index_found)
{
	for (size_t i = 0; i < rip_ctx->rip_ifs_count; ++i) {
		const rip_if_entry *entry = &rip_ctx->rip_ifs[i];
		if (entry->fd == fd) {
			*index_found = i;
			return 0;
		}
	}

	return 1;
}

int rip_begin(rip_context *rip_ctx)
{
	LOG_INFO("%s", __func__);

	if (setup_resources(rip_ctx)) {
		LOG_ERR("failed to setup_resources");
		return 1;
	}

	LOG_INFO("Waiting for messages...");

	struct pollfd pollfds[POLL_FDS_MAX_SIZE];
	MEMSET_ZERO(pollfds);

	while (1) {
		nfds_t actual_pollfds_count = 0;
		assign_fds_to_pollfds(rip_ctx, pollfds, POLL_FDS_MAX_SIZE,
				      &actual_pollfds_count);
		assert("actual_pollfds_count" && actual_pollfds_count > 0);

		if (-1 == poll(pollfds, actual_pollfds_count, -1)) {
			LOG_ERR("poll failed: %s", strerror(errno));
		}

		for (nfds_t i = 0; i < actual_pollfds_count; ++i) {
			const struct pollfd *current_pollfd = &pollfds[i];
			if (!(current_pollfd->revents & POLLIN)) {
				continue;
			}

			size_t rip_if_entry_index = 0;
			if (!rip_if_entry_find_by_fd(rip_ctx,
						     current_pollfd->fd,
						     &rip_if_entry_index)) {
				rip_handle_io(rip_ctx, rip_if_entry_index);
			} else {
				LOG_ERR("Can't dispatch this event, fd: %d, "
					"revents: %d",
					current_pollfd->fd,
					current_pollfd->revents);
				return 1;
			}
		}
	}

	return 0;
}