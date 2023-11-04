#include "rip.h"
#include "logging.h"
#include "rip_ipc.h"
#include "rip_messages.h"

#include "rip_handle_resp.h"
#include "rip_if.h"
#include "rip_route.h"
#include "utils.h"
#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>

#define RIP_PORT 520
#define POLL_FDS_MAX_LEN 128

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
	if (set_nonblocking(if_entry->fd)) {
		return 1;
	}
	if (bind_to_device(if_entry->fd, if_entry->if_name)) {
		return 1;
	}
	if (set_allow_reuse_port(if_entry->fd)) {
		return 1;
	}
	if (bind_port(if_entry->fd)) {
		return 1;
	}
	if (join_multicast(if_entry->fd, if_entry->if_index)) {
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

struct msg_buffer {
	struct rip_header header;
	struct rip2_entry entries[500];
};

int rip_handle_io(rip_context *rip_ctx, const size_t rip_if_entry_idx)
{
	const rip_if_entry *rip_if_e = &rip_ctx->rip_ifs[rip_if_entry_idx];

	struct msg_buffer msg_buffer;
	struct sockaddr_in addr_n;
	socklen_t addrlen = sizeof(addr_n);
	ssize_t nbytes = recvfrom(rip_if_e->fd, &msg_buffer, sizeof(msg_buffer),
				  0, (struct sockaddr *)&addr_n, &addrlen);
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

	// rip_header_print(&msg_buffer.header);
	nbytes -= sizeof(msg_buffer.header);

	if (msg_buffer.header.command == RIP_CMD_RESPONSE) {
		const size_t n_entry = nbytes / sizeof(struct rip2_entry);
		if (handle_response(rip_ctx, msg_buffer.entries, n_entry,
				    addr_n.sin_addr)) {
			LOG_ERR("Failed to handle response");
			return 1;
		}
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

	size_t i = 0;
	for (; i < rip_ctx->rip_ifs_count; ++i) {
		pollfds[i].fd	  = rip_ctx->rip_ifs[i].fd;
		pollfds[i].events = POLLIN;
	}
	pollfds[i++] = (struct pollfd){
	    .fd	    = rip_route_getfd(rip_ctx->route_mngr),
	    .events = POLLIN,
	};
	pollfds[i++] = (struct pollfd){
	    .fd	    = rip_ipc_getfd(rip_ctx->ipc_mngr),
	    .events = POLLIN,
	};

	*actual_pollfds_count = i;
}

int rip_if_entry_find_by_fd(const rip_context *rip_ctx, const int fd,
			    size_t *rip_ifs_idx)
{
	for (size_t i = 0; i < rip_ctx->rip_ifs_count; ++i) {
		const rip_if_entry *entry = &rip_ctx->rip_ifs[i];
		if (entry->fd == fd) {
			*rip_ifs_idx = i;
			return 0;
		}
	}

	return 1;
}

int rip_begin(rip_context *rip_ctx)
{
	LOG_INFO("%s", __func__);

	rip_ctx->route_mngr = rip_route_alloc_init();
	if (!rip_ctx->route_mngr) {
		return 1;
	}

	//rip_route_print_table(rip_ctx->route_mngr);

	struct in_addr dest;
	struct in_addr next_hop;
	inet_pton(AF_INET, "10.120.3.0", &dest);
	inet_pton(AF_INET, rip_ctx->rip_ifs[0].if_addr, &next_hop);

	rip_route_entry *entry = rip_route_entry_create(
	    dest, 24, rip_ctx->rip_ifs[0].if_index, next_hop);
	if (!entry) {
		LOG_ERR("rip_route_entry_create");
	}

	if(rip_route_add_route(rip_ctx->route_mngr, entry) > 0) {
		LOG_ERR("rip_route_add_route");
		return 1;
	}
	

	rip_ctx->ipc_mngr = rip_ipc_alloc();
	if (!rip_ctx->ipc_mngr) {
		return 1;
	}
	struct r_ipc_cmd_handler handlers[] = {
	    [0] = {.cmd	 = dump_routing_table,
		   .data = rip_ctx->route_mngr,
		   .cb	 = rip_route_sprintf_table}};

	if (rip_ipc_init(rip_ctx->ipc_mngr, handlers, ARRAY_LEN(handlers))) {
		return 1;
	}

	if (setup_resources(rip_ctx)) {
		LOG_ERR("failed to setup_resources");
		return 1;
	}

	LOG_INFO("Waiting for events...");

	struct pollfd pollfds[POLL_FDS_MAX_LEN];
	MEMSET_ZERO(&pollfds);

	while (1) {
		nfds_t actual_pollfds_count = 0;
		assign_fds_to_pollfds(rip_ctx, pollfds, POLL_FDS_MAX_LEN,
				      &actual_pollfds_count);
		assert("actual_pollfds_count" && actual_pollfds_count > 0);

		if (-1 == poll(pollfds, actual_pollfds_count, -1)) {
			LOG_ERR("poll failed: %s", strerror(errno));
		}

		for (nfds_t i = 0; i < actual_pollfds_count; ++i) {
			int revents = pollfds[i].revents;
			int fd	    = pollfds[i].fd;

			if (!(revents & POLLIN)) {
				continue;
			}
			LOG_INFO("event on fd: %d", fd);

			size_t rip_ifs_idx = 0;
			if (rip_route_getfd(rip_ctx->route_mngr) == fd) {
				LOG_INFO("rip route update");
				rip_route_handle_netlink_io(
				    rip_ctx->route_mngr);
			} else if (rip_ipc_getfd(rip_ctx->ipc_mngr) == fd) {
				LOG_INFO("rip_ipc_handle_msg");
				rip_ipc_handle_msg(rip_ctx->ipc_mngr);
			} else if (!rip_if_entry_find_by_fd(rip_ctx, fd,
							    &rip_ifs_idx)) {
				rip_handle_io(rip_ctx, rip_ifs_idx);
			} else {
				LOG_ERR("Can't dispatch this event, fd: %d, "
					"revents: %d",
					fd, revents);
				return 1;
			}
		}
	}

	return 0;
}
