#include "rip.h"
#include "rip_common.h"
#include "rip_db.h"
#include "rip_ipc.h"
#include "rip_recv.h"
#include "rip_route.h"
#include "rip_socket.h"
#include "rip_update.h"
#include "utils/config/parse_rip_config.h"
#include "utils/event.h"
#include "utils/event_dispatcher.h"
#include "utils/logging.h"
#include "utils/timer.h"
#include "utils/utils.h"
#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>

#define RIP_CONFIG_FILENAME "/etc/rip/config.yaml"

int rip_handle_t_update(const struct event *event)
{
	struct rip_context *ctx = event->arg;
	if (timer_clear(&ctx->timers.t_update)) {
		return 1;
	}

	return rip_send_advertisement_multicast(ctx, false);
}

int rip_handle_t_triggered_lock(const struct event *event)
{
	LOG_INFO("rip_handle_t_triggered_lock");

	struct rip_context *ctx = event->arg;
	if (timer_clear(&ctx->timers.t_triggered_lock)) {
		return 1;
	}

	ctx->timers.t_triggered_lock_expired = true;
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

	if (event_dispatcher_register(ed, &(struct event){.fd  = ctx->timers.t_update.fd,
							  .cb  = rip_handle_t_update,
							  .arg = ctx})) {
		return 1;
	}

	if (event_dispatcher_register(ed, &(struct event){.fd  = ctx->timers.t_triggered_lock.fd,
							  .cb  = rip_handle_t_triggered_lock,
							  .arg = ctx})) {
		return 1;
	}

	return 0;
}

int handle_route_changed(struct rip_context *ctx)
{
	if (ctx->timers.t_triggered_lock_expired == false) {
		LOG_INFO("timer lock pending");
		return 0;
	}

	LOG_INFO("handling state changed...");
	bool advertise_only_changed = true;
	if (rip_send_advertisement_multicast(ctx, advertise_only_changed)) {
		LOG_ERR("rip_send_advertisement_multicast");
		return 1;
	}

	//TODO randomize value between 2 and 5 seconds
	if (timer_start_oneshot(&ctx->timers.t_triggered_lock, 0.1f)) {
		LOG_ERR("timer_start_oneshot");
		return 1;
	}
	ctx->timers.t_triggered_lock_expired = false;

	return 0;
}

static int rip_handle_events(struct rip_context *rip_ctx)
{
	LOG_INFO("Waiting for events...");

	while (1) {
		if (event_dispatcher_poll_and_dispatch(&rip_ctx->event_dispatcher)) {
			return 1;
		}

		// todo: fix global flag setting
		if (rip_ctx->state == rip_state_route_changed) {
			if (handle_route_changed(rip_ctx)) {
				return 1;
			}
		}
		rip_ctx->state = rip_state_idle;
	}

	return 0;
}

static int add_advertised_network_to_db(struct rip_db *db,
					const struct advertised_network *adv_network)
{
	struct rip_route_description desc;
	MEMSET_ZERO(&desc);

	desc.learned_via = rip_route_learned_via_conf,
	desc.if_index	 = if_nametoindex(adv_network->dev);
	if (desc.if_index == 0) {
		LOG_ERR("if_nametoindex failed, dev: %s, errno %s", adv_network->dev,
			strerror(errno));
		return 1;
	}

	struct rip2_entry *entry = &desc.entry;
	if (1 != inet_pton(AF_INET, adv_network->address, &entry->ip_address.s_addr)) {
		LOG_ERR("inet_pton: address: %s, errno: %s", adv_network->address, strerror(errno));
		return 1;
	}
	if (prefix_len_to_subnet(*adv_network->prefix, &entry->subnet_mask)) {
		return 1;
	}

	entry->ip_address.s_addr = entry->ip_address.s_addr & entry->subnet_mask.s_addr;
	entry->next_hop.s_addr	 = 0;
	entry->routing_family_id = AF_INET;
	entry->route_tag	 = 10; // arbitrary number
	entry->metric		 = 1;

	if (rip_db_add(db, &desc)) {
		return 1;
	}

	return 0;
}

static int add_advertised_networks_to_db(struct rip_db *db, const struct rip_configuration *conf)
{
	for (size_t i = 0; i < conf->advertised_networks_n; ++i) {
		const struct advertised_network *adv_network = &conf->advertised_networks[i];
		if (add_advertised_network_to_db(db, adv_network)) {
			return 1;
		}
	}
	return 0;
}

int rip_init_timers(struct rip_timers *timers)
{
	if (timer_init(&timers->t_update) || timer_start_interval(&timers->t_update, 30, 10)) {
		LOG_ERR("t_update");
		return 1;
	}

	if (timer_init(&timers->t_triggered_lock)) {
		LOG_ERR("t_triggered_lock");
		return 1;
	}
	timers->t_triggered_lock_expired = true;

	return 0;
}

int rip_begin(struct rip_context *ctx)
{
	LOG_INFO("%s", __func__);

	if (rip_read_config(RIP_CONFIG_FILENAME, &ctx->config)) {
		LOG_ERR("failed to read configuration");
		return 1;
	}
	rip_configuration_print(&ctx->config);

	if (rip_set_if_index_to_sockets(&ctx->config, &ctx->rip_ifcs, &ctx->rip_ifcs_n)) {
		return 1;
	}

	if (rip_setup_sockets(ctx->rip_ifcs, ctx->rip_ifcs_n)) {
		LOG_ERR("failed to setup_resources");
		return 1;
	}
	rip_print_sockets(ctx->rip_ifcs, ctx->rip_ifcs_n);

	ctx->route_mngr = rip_route_alloc_init();
	if (!ctx->route_mngr) {
		return 1;
	}

	if (rip_db_init(&ctx->rip_db)) {
		return 1;
	}
	if (add_advertised_networks_to_db(&ctx->rip_db, &ctx->config)) {
		return 1;
	}

	struct r_ipc_cmd_handler handlers[] = {
	    {.cmd = dump_libnl_route_table, .data = ctx->route_mngr, .cb = rip_route_sprintf_table},
	    {.cmd = dump_rip_routes, .data = &ctx->rip_db, .cb = rip_db_dump}};

	ctx->ipc_mngr = rip_ipc_alloc_init(handlers, ARRAY_LEN(handlers));
	if (!ctx->ipc_mngr) {
		return 1;
	}

	rip_init_timers(&ctx->timers);

	if (init_event_dispatcher(ctx)) {
		LOG_ERR("init_event_dispatcher");
		return 1;
	}

	if (rip_send_request_multicast(ctx)) {
		return 1;
	}

	return rip_handle_events(ctx);
}
