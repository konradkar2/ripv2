#include "rip.h"
#include "config/parse_rip_config.h"
#include "logging.h"
#include "rip_db.h"
#include "rip_ipc.h"
#include "rip_messages.h"

#include "rip_handle_resp.h"
#include "rip_if.h"
#include "rip_route.h"
#include "utils.h"
#include "utils/vector.h"
#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>

#define RIP_PORT 520
#define RIP_CONFIG_FILENAME "/etc/rip/config.yaml"

static void rip_if_entry_print(FILE *output, const struct rip_ifc *e)
{
	fprintf(output, "fd: %d, if_name: %s, if_index : %d\n", e->fd, e->if_name, e->if_index);
}

static int rip_if_entry_setup_resources(struct rip_ifc *if_entry)
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

static int setup_resources(struct rip_context *rip_ctx)
{
	LOG_INFO("%s", __func__);

	for (size_t i = 0; i < rip_ctx->rip_ifcs_n; ++i) {
		struct rip_ifc *entry = &rip_ctx->rip_ifcs[i];

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

int rip_handle_io(struct rip_context *rip_ctx, const size_t rip_if_entry_idx)
{
	const struct rip_ifc *rip_if_e = &rip_ctx->rip_ifcs[rip_if_entry_idx];

	struct msg_buffer msg_buffer;
	struct sockaddr_in sender_addr;
	socklen_t sender_addr_len = sizeof(sender_addr);
	ssize_t nbytes = recvfrom(rip_if_e->fd, &msg_buffer, sizeof(msg_buffer), 0, (struct sockaddr *)&sender_addr,
				  &sender_addr_len);
	if (nbytes < 0) {
		LOG_ERR("recvfrom failed: %s", strerror(errno));
		return 1;
	}

	char addr_h[INET_ADDRSTRLEN];
	if (NULL != inet_ntop(AF_INET, &sender_addr.sin_addr, addr_h, INET_ADDRSTRLEN)) {
		LOG_INFO("Got message from %s on %s interface", addr_h, rip_if_e->if_name);
	} else {
		LOG_ERR("got message, but inet_ntop failed: %s", strerror(errno));
		return 1;
	}

	// rip_header_print(&msg_buffer.header);
	nbytes -= sizeof(msg_buffer.header);

	if (msg_buffer.header.command == RIP_CMD_RESPONSE) {
		const size_t n_entry = nbytes / sizeof(struct rip2_entry);
		if (handle_response(rip_ctx->route_mngr, &rip_ctx->rip_db, msg_buffer.entries, n_entry,
				    sender_addr.sin_addr, rip_if_e->if_index)) {
			LOG_ERR("Failed to handle response");
			return 1;
		}
	}

	return 0;
}

static int assign_fds_to_pollfds(const struct rip_context *rip_ctx, struct vector *pollfds_vec)
{

	for (size_t i = 0; i < rip_ctx->rip_ifcs_n; ++i) {
		if (vector_add(pollfds_vec, &(struct pollfd){
						.fd	= rip_ctx->rip_ifcs[i].fd,
						.events = POLLIN,
					    })) {
			return 1;
		}
	}
	if (vector_add(pollfds_vec, &(struct pollfd){
					.fd	= rip_route_getfd(rip_ctx->route_mngr),
					.events = POLLIN,
				    })) {
		return 1;
	};

	if (vector_add(pollfds_vec, &(struct pollfd){
					.fd	= rip_ipc_getfd(rip_ctx->ipc_mngr),
					.events = POLLIN,
				    })) {
		return 1;
	}

	return 0;
}

int rip_if_entry_find_by_fd(const struct rip_context *rip_ctx, const int fd, size_t *rip_ifs_idx)
{
	for (size_t i = 0; i < rip_ctx->rip_ifcs_n; ++i) {
		const struct rip_ifc *entry = &rip_ctx->rip_ifcs[i];
		if (entry->fd == fd) {
			*rip_ifs_idx = i;
			return 0;
		}
	}

	return 1;
}

static int rip_read_config(struct rip_configuration *rip_cfg)
{
	int ret		  = 0;
	FILE *config_file = NULL;

	config_file = fopen(RIP_CONFIG_FILENAME, "r");
	if (!config_file) {
		LOG_ERR("fopen %s: %s", RIP_CONFIG_FILENAME, strerror(errno));
		return 1;
	}

	if (rip_configuration_read_and_parse(config_file, rip_cfg)) {
		LOG_ERR("failed to parse configuration");
		ret = 1;
	}
	if (rip_configuration_validate(rip_cfg)) {
		LOG_ERR("invalid configuration");
		ret = 1;
	}

	fclose(config_file);
	return ret;
}

static int rip_assign_ifcs(struct rip_configuration *cfg, struct rip_ifc **ifcs, size_t *ifcs_n)
{
	*ifcs = CALLOC(sizeof(struct rip_ifc) * cfg->rip_interfaces_n);
	if (!(*ifcs)) {
		return 1;
	}
	*ifcs_n = cfg->rip_interfaces_n;

	for (size_t i = 0; i < cfg->rip_interfaces_n; ++i) {
		struct rip_interface *interface_cfg = &cfg->rip_interfaces[i];
		struct rip_ifc *ifc		    = &(*ifcs)[i];

		strncpy(ifc->if_name, interface_cfg->dev, IF_NAMESIZE);
		if (ifc->if_name[0] == '\0') {
			LOG_ERR("Invalid dev name");
			return 1;
		}

		ifc->if_index = if_nametoindex(ifc->if_name);
		if (ifc->if_index == 0) {
			LOG_ERR("if_nametoindex failed (%s): %s", ifc->if_name, strerror(errno));
			return 1;
		}
	}

	return 0;
}

static int rip_handle_events(struct rip_context *rip_ctx)
{
	LOG_INFO("Waiting for events...");
	struct vector *pollfds_vec = NULL;

	while (1) {
		vector_free(pollfds_vec);
		pollfds_vec = vector_create(rip_ctx->rip_ifcs_n + 10, sizeof(struct pollfd));
		if (!pollfds_vec) {
			return 1;
		}

		if (assign_fds_to_pollfds(rip_ctx, pollfds_vec)) {
			return 1;
		}

		if (-1 == poll(vector_get(pollfds_vec, 0), vector_get_len(pollfds_vec), -1)) {
			LOG_ERR("poll failed: %s", strerror(errno));
		}

		for (size_t i = 0; i < vector_get_len(pollfds_vec); ++i) {
			struct pollfd *pollfd = vector_get(pollfds_vec, i);

			int revents = pollfd->revents;
			int fd	    = pollfd->fd;

			if (!(revents & POLLIN)) {
				continue;
			}

			LOG_INFO("event on fd: %d", fd);
			size_t rip_ifs_idx = 0;
			if (rip_route_getfd(rip_ctx->route_mngr) == fd) {
				LOG_INFO("rip route update");
				rip_route_handle_netlink_io(rip_ctx->route_mngr);
			} else if (rip_ipc_getfd(rip_ctx->ipc_mngr) == fd) {
				LOG_INFO("rip_ipc_handle_msg");
				rip_ipc_handle_msg(rip_ctx->ipc_mngr);
			} else if (!rip_if_entry_find_by_fd(rip_ctx, fd, &rip_ifs_idx)) {
				rip_handle_io(rip_ctx, rip_ifs_idx);
			} else {
				LOG_ERR("Can't dispatch this event, fd: %d, "
					"revents: %d",
					fd, revents);
				return 1;
			}
		}
	}
}

int rip_begin(struct rip_context *rip_ctx)
{
	LOG_INFO("%s", __func__);

	if (rip_read_config(&rip_ctx->config)) {
		LOG_ERR("failed to read configuration");
		return 1;
	}
	rip_configuration_print(&rip_ctx->config);

	if (rip_assign_ifcs(&rip_ctx->config, &rip_ctx->rip_ifcs, &rip_ctx->rip_ifcs_n)) {
		return 1;
	}

	rip_ctx->route_mngr = rip_route_alloc_init();
	if (!rip_ctx->route_mngr) {
		return 1;
	}

	rip_ctx->ipc_mngr = rip_ipc_alloc();
	if (!rip_ctx->ipc_mngr) {
		return 1;
	}

	if (rip_db_init(&rip_ctx->rip_db) > 0) {
		return 1;
	}

	struct r_ipc_cmd_handler handlers[] = {
	    {.cmd = dump_libnl_route_table, .data = rip_ctx->route_mngr, .cb = rip_route_sprintf_table},
	    {.cmd = dump_rip_routes, .data = &rip_ctx->rip_db, .cb = rip_db_dump}};

	if (rip_ipc_init(rip_ctx->ipc_mngr, handlers, ARRAY_LEN(handlers))) {
		return 1;
	}

	if (setup_resources(rip_ctx)) {
		LOG_ERR("failed to setup_resources");
		return 1;
	}

	return rip_handle_events(rip_ctx);
}
