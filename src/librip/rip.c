#include "rip.h"
#include "rip_common.h"
#include "rip_db.h"
#include "rip_handle_resp.h"
#include "rip_ipc.h"
#include "rip_route.h"
#include "rip_update.h"
#include "utils/config/parse_rip_config.h"
#include "utils/event.h"
#include "utils/event_dispatcher.h"
#include "utils/hashmap.h"
#include "utils/logging.h"
#include "utils/network/socket.h"
#include "utils/timer.h"
#include "utils/utils.h"
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

#define RIP_CONFIG_FILENAME "/etc/rip/config.yaml"

static void rip_socket_print(FILE *output, const struct rip_socket *e)
{
	fprintf(output, "fd: %d, if_name: %s, if_index : %d\n", e->fd, e->if_name, e->if_index);
}

static int setup_socket_rx(struct rip_socket *if_entry)
{
	if (socket_create_udp_socket(&if_entry->fd)) {
		return 1;
	}
	if (socket_set_nonblocking(if_entry->fd)) {
		return 1;
	}
	if (socket_bind_to_device(if_entry->fd, if_entry->if_name)) {
		return 1;
	}
	if (socket_set_allow_reuse_port(if_entry->fd)) {
		return 1;
	}
	if (socket_bind_port(if_entry->fd, RIP_UDP_PORT)) {
		return 1;
	}
	if (socket_join_multicast(if_entry->fd, if_entry->if_index, RIP_MULTICAST_ADDR)) {
		return 1;
	}

	return 0;
}

static int setup_socket_tx(struct rip_socket *if_entry)
{
	if (socket_create_udp_socket(&if_entry->fd)) {
		return 1;
	}
	if (socket_bind_to_device(if_entry->fd, if_entry->if_name)) {
		return 1;
	}
	if (socket_set_allow_reuse_port(if_entry->fd)) {
		return 1;
	}
	if (socket_bind_port(if_entry->fd, RIP_UDP_PORT)) {
		return 1;
	}
	if(socket_disable_multicast_loopback(if_entry->fd)) {
		return 1;
	}

	return 0;
}

static int rip_setup_sockets(struct rip_ifc *ifcs, size_t ifcs_n)
{
	LOG_INFO("%s", __func__);

	for (size_t i = 0; i < ifcs_n; ++i) {
		struct rip_ifc *ifc = &ifcs[i];

		{
			struct rip_socket *socket_rx = &ifc->socket_rx;
			if (setup_socket_rx(socket_rx)) {
				LOG_ERR("setup_socket_rx");
				rip_socket_print(stderr, socket_rx);
				return 1;
			}
		}
		{
			struct rip_socket *socket_tx = &ifc->socket_tx;
			if (setup_socket_tx(socket_tx)) {
				LOG_ERR("setup_socket_tx");
				rip_socket_print(stderr, socket_tx);
				return 1;
			}
		}
	}

	return 0;
}

struct rip_socket *rip_find_rx_socket_by_fd(struct rip_ifc *ifcs, size_t entries_n, const int fd)
{
	for (size_t i = 0; i < entries_n; ++i) {
		struct rip_ifc *ifc = &ifcs[i];
		if (ifc->socket_rx.fd == fd) {
			return &ifc->socket_rx;
		}
	}

	return NULL;
}

int rip_receive_messages(int fd, struct msg_buffer *buff, struct in_addr *sender_address,
			 size_t *bytes)
{
	struct sockaddr_in sa_in;
	MEMSET_ZERO(&sa_in);
	socklen_t sender_addr_len = sizeof(sa_in);

	ssize_t nbytes =
	    recvfrom(fd, buff, sizeof(*buff), 0, (struct sockaddr *)&sa_in, &sender_addr_len);
	if (nbytes < 0) {
		LOG_ERR("recvfrom failed: %s", strerror(errno));
		return 1;
	}

	sender_address->s_addr = sa_in.sin_addr.s_addr;
	*bytes		       = nbytes;

	return 0;
}

size_t calculate_entries_count(size_t payload_size_bytes)
{
	return (payload_size_bytes - sizeof(struct rip_header)) / sizeof(struct rip2_entry);
}

int rip_handle_message_event(const struct event *event)
{
	struct rip_context *rip_ctx = event->arg;
	const struct rip_socket *socket =
	    rip_find_rx_socket_by_fd(rip_ctx->rip_ifcs, rip_ctx->rip_ifcs_n, event->fd);

	if (!socket) {
		BUG();
	}

	struct msg_buffer msg_buffer;
	struct in_addr sender;
	size_t n_bytes;
	if (rip_receive_messages(socket->fd, &msg_buffer, &sender, &n_bytes)) {
		return 1;
	}

	switch (msg_buffer.header.command) {
	case RIP_CMD_RESPONSE:
		return rip_handle_response(rip_ctx->route_mngr, &rip_ctx->rip_db,
					   msg_buffer.entries, calculate_entries_count(n_bytes),
					   sender, socket->if_index, &rip_ctx->state);
	case RIP_CMD_REQUEST:
		LOG_ERR("RIP_CMD_REQUEST: unimplemented");
		return 1;
		break;
	default:
		LOG_ERR("Unsupported rip command: %d", msg_buffer.header.command);
		return 1;
	}
}

static int rip_read_config(struct rip_configuration *rip_cfg)
{
	int ret		  = 0;
	FILE *config_file = NULL;

	config_file = fopen(RIP_CONFIG_FILENAME, "r");
	if (!config_file) {
		LOG_ERR("fopen %s: %s", RIP_CONFIG_FILENAME, strerror(errno));
		ret = 1;
		goto cleanup;
	}

	if (rip_configuration_read_and_parse(config_file, rip_cfg)) {
		LOG_ERR("failed to parse configuration");
		ret = 1;
		goto cleanup;
	}
	if (rip_configuration_validate(rip_cfg)) {
		LOG_ERR("invalid configuration");
		ret = 1;
		goto cleanup;
	}
cleanup:
	fclose(config_file);
	return ret;
}

static int rip_assign_ifc_to_socket(struct rip_interface *interface_cfg, struct rip_socket *socket)
{
	strncpy(socket->if_name, interface_cfg->dev, sizeof(socket->if_name));
	if (socket->if_name[0] == '\0') {
		LOG_ERR("Invalid dev name");
		return 1;
	}

	socket->if_index = if_nametoindex(socket->if_name);
	if (socket->if_index == 0) {
		LOG_ERR("if_nametoindex failed (%s): %s", socket->if_name, strerror(errno));
		return 1;
	}

	return 0;
}

static int rip_assign_ifcs_to_sockets(struct rip_configuration *cfg, struct rip_ifc **rip_ifcs,
				      size_t *rip_ifcs_n)
{
	*rip_ifcs = CALLOC(sizeof(struct rip_ifc) * cfg->rip_interfaces_n);
	if (!(*rip_ifcs)) {
		return 1;
	}
	*rip_ifcs_n = cfg->rip_interfaces_n;

	for (size_t i = 0; i < cfg->rip_interfaces_n; ++i) {
		struct rip_interface *interface_cfg = &cfg->rip_interfaces[i];
		struct rip_ifc *ifc		    = *rip_ifcs;

		struct rip_socket *socket_rx = &ifc[i].socket_rx;
		struct rip_socket *socket_tx = &ifc[i].socket_tx;

		rip_assign_ifc_to_socket(interface_cfg, socket_rx);
		rip_assign_ifc_to_socket(interface_cfg, socket_tx);
	}

	return 0;
}

int init_event_dispatcher(struct rip_context *ctx)
{
	struct event_dispatcher *ed = &ctx->event_dispatcher;
	if (event_dispatcher_init(ed)) {
		return 1;
	}

	for (size_t i = 0; i < ctx->rip_ifcs_n; ++i) {
		const struct rip_socket *socket = &ctx->rip_ifcs[i].socket_rx;
		if (event_dispatcher_register(
			ed,
			&(struct event){.fd = socket->fd, .cb = rip_handle_message_event, ctx})) {
			return 1;
		}
	}

	if (event_dispatcher_register(ed, &(struct event){.fd  = rip_ipc_getfd(ctx->ipc_mngr),
							  .cb  = rip_ipc_handle_event,
							  .arg = ctx->ipc_mngr})) {
		return 1;
	}

	if (event_dispatcher_register(ed, &(struct event){.fd  = rip_route_getfd(ctx->route_mngr),
							  .cb  = rip_route_handle_event,
							  .arg = ctx->route_mngr})) {
		return 1;
	}

	if (event_dispatcher_register(
		ed,
		&(struct event){.fd = ctx->t_update.fd, .cb = rip_handle_t_update, .arg = ctx})) {
		return 1;
	}

	return 0;
}

static int rip_handle_events(struct rip_context *rip_ctx)
{
	LOG_INFO("Waiting for events...");

	while (1) {
		if (event_dispatcher_poll_and_dispatch(&rip_ctx->event_dispatcher)) {
			return 1;
		}

		if (rip_ctx->state == rip_state_route_changed) {
			// do something;
		}
		rip_ctx->state = rip_state_idle;
	}

	return 0;
}

int rip_begin(struct rip_context *ctx)
{
	LOG_INFO("%s", __func__);

	if (rip_read_config(&ctx->config)) {
		LOG_ERR("failed to read configuration");
		return 1;
	}
	rip_configuration_print(&ctx->config);

	if (rip_assign_ifcs_to_sockets(&ctx->config, &ctx->rip_ifcs, &ctx->rip_ifcs_n)) {
		return 1;
	}

	if (rip_setup_sockets(ctx->rip_ifcs, ctx->rip_ifcs_n)) {
		LOG_ERR("failed to setup_resources");
		return 1;
	}

	ctx->route_mngr = rip_route_alloc_init();
	if (!ctx->route_mngr) {
		return 1;
	}

	ctx->ipc_mngr = rip_ipc_alloc();
	if (!ctx->ipc_mngr) {
		return 1;
	}

	if (rip_db_init(&ctx->rip_db) > 0) {
		return 1;
	}

	struct r_ipc_cmd_handler handlers[] = {
	    {.cmd = dump_libnl_route_table, .data = ctx->route_mngr, .cb = rip_route_sprintf_table},
	    {.cmd = dump_rip_routes, .data = &ctx->rip_db, .cb = rip_db_dump}};

	if (rip_ipc_init(ctx->ipc_mngr, handlers, ARRAY_LEN(handlers))) {
		return 1;
	}

	if (timer_init(&ctx->t_update) || timer_start_interval(&ctx->t_update, 30)) {
		return 1;
	}

	if (init_event_dispatcher(ctx)) {
		LOG_ERR("init_event_dispatcher");
		return 1;
	}

	return rip_handle_events(ctx);
}
