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
#include <bits/time.h>
#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>

#define RIP_CONFIG_FILENAME "/etc/rip/config.yaml"
#define TIMEOUT_PERIOD_TIMER 15
#define TIMEOUT_PERIOD_RFC 180
#define GC_PERIOD_TIMER 5
#define GC_PERIOD_RFC 120

static float create_t_update_time(void)
{
	// 3.8 Timers: 30 seconds +/- random (0,5) seconds
	return 27.5 + get_random_float(2.5, 7.5);
}

static int rip_t_update_expired(const struct event *event)
{
	struct rip_context *ctx = event->arg;
	if (timer_clear(&ctx->timers.t_update)) {
		PANIC();
	}

	if (rip_send_advertisement_multicast(ctx->rip_db, &ctx->rip_ifcs, false)) {
		LOG_ERR("rip_send_advertisement_multicast");
		return 1;
	}

	if (timer_start_oneshot(&ctx->timers.t_update, create_t_update_time())) {
		LOG_ERR("timer_start_oneshot");
		PANIC();
	}

	return 0;
}

static int rip_t_request_warmup_expired(const struct event *event)
{
	struct rip_context *ctx = event->arg;
	if (timer_clear(&ctx->timers.t_request_warmup)) {
		PANIC();
	}

	rip_send_request_multicast(&ctx->rip_ifcs);
	return 0;
}

static int rip_t_triggered_lock_expired(const struct event *event)
{
	LOG_INFO("%s", __func__);

	struct timer *t_triggered_lock = event->arg;
	if (timer_clear(t_triggered_lock)) {
		PANIC();
	}
	return 0;
}

static int rip_route_timeout_expired(struct rip_route_mngr *route_mngr, struct rip_db *db,
				     struct rip_route_description *route,
				     const struct timespec	   expired_at)
{
	LOG_INFO("route timeout expired: ");
	rip_route_description_print(route, stdout);

	if (rip_route_delete_route(route_mngr, route)) {
		LOG_ERR("failed to remove route from routing table, removing it "
			"from database");
		if (rip_db_remove(db, route)) {
			PANIC();
		}
		return 1;
	}

	route->changed		= true;
	route->entry.metric	= htonl(16);
	route->in_routing_table = false;
	route->gc_started_at	= expired_at;

	if (rip_db_move_to_garbage(db, route)) {
		return 1;
	}

	return 0;
}

static int rip_t_timeout_expired(const struct event *event)
{
	struct rip_context *ctx = event->arg;
	if (timer_clear(&ctx->timers.t_timeout)) {
		PANIC();
	}

	struct rip_db_iter	      iter  = {0};
	struct rip_route_description *route = NULL;

	struct timespec now = {0};
	if (clock_gettime(CLOCK_MONOTONIC, &now)) {
		LOG_ERR("clock_gettime: %s", strerror(errno));
		PANIC();
	}

	bool start_gc = false;
	while (rip_db_iter(ctx->rip_db, rip_db_ok, &iter, &route)) {
		if (route->is_local) {
			continue;
		}

		route->timeout_cnt += TIMEOUT_PERIOD_TIMER;
		if (route->timeout_cnt >= TIMEOUT_PERIOD_RFC) {

			if (rip_route_timeout_expired(ctx->route_mngr, ctx->rip_db, route, now)) {
				return 1;
			}

			start_gc = true;
			iter	 = (struct rip_db_iter){0};
		}
	}

	if (start_gc) {
		if (!timer_is_ticking(&ctx->timers.t_gc)) {
			LOG_DEBUG("starting garbage collection timer");
			timer_start_oneshot(&ctx->timers.t_gc, GC_PERIOD_TIMER);
		}
	}

	if (timer_start_oneshot(&ctx->timers.t_timeout, TIMEOUT_PERIOD_TIMER)) {
		PANIC();
	}

	return 0;
}

static bool has_time_passed(const struct timespec *now, const struct timespec *then, int time)
{
	return (now->tv_sec - then->tv_sec) >= time;
}

static int rip_t_gc_expired(const struct event *event)
{
	struct rip_context *ctx = event->arg;
	if (timer_clear(&ctx->timers.t_gc)) {
		PANIC();
	}

	struct rip_route_description *route = NULL;

	struct timespec now = {0};
	if (clock_gettime(CLOCK_MONOTONIC, &now)) {
		LOG_ERR("clock_gettime: %s", strerror(errno));
		PANIC();
	}

	struct rip_db_iter iter = {0};
	while (rip_db_iter(ctx->rip_db, rip_db_garbage, &iter, &route)) {
		if (has_time_passed(&now, &route->gc_started_at, GC_PERIOD_RFC)) {
			if (rip_db_remove(ctx->rip_db, route)) {
				LOG_ERR("failed to remove route");
				return 1;
			}
			iter = (struct rip_db_iter){0};
		}
	}

	if (rip_db_count(ctx->rip_db, rip_db_garbage) > 0) {
		if (timer_start_oneshot(&ctx->timers.t_gc, GC_PERIOD_TIMER)) {
			PANIC();
		}
	}

	return 0;
}

int check_route_changed(struct rip_context *ctx)
{
	if (timer_is_ticking(&ctx->timers.t_triggered_lock)) {
		return 0;
	}

	if (rip_db_any_route_changed(ctx->rip_db)) {

		bool advertise_only_changed = true;
		if (rip_send_advertisement_multicast(ctx->rip_db, &ctx->rip_ifcs,
						     advertise_only_changed)) {
			LOG_ERR("rip_send_advertisement_multicast");
			return 1;
		}

		// 3.10.1 triggered update lock time
		if (timer_start_oneshot(&ctx->timers.t_triggered_lock, get_random_float(1, 5))) {
			LOG_ERR("timer_start_oneshot");
			return 1;
		}
	}

	return 0;
}

static int init_event_dispatcher(struct rip_context *ctx)
{
	struct event_dispatcher *ed = event_dispatcher_init();
	if (!ed) {
		return 1;
	}
	ctx->event_dispatcher = ed;

	for (size_t i = 0; i < ctx->rip_ifcs.count; ++i) {
		const struct rip_socket *socket = &ctx->rip_ifcs.items[i].socket_rx;
		if (event_dispatcher_register(ed, &(struct event){.fd = socket->fd,
								  .cb = rip_handle_message_event,
								  ctx,
								  socket->if_name})) {
			return 1;
		}
	}

	struct event events[] = {
	    {rip_ipc_getfd(ctx->ipc_mngr), rip_ipc_handle_event, ctx->ipc_mngr, "ipc"},
	    {rip_route_getfd(ctx->route_mngr), rip_route_handle_event, ctx->route_mngr,
	     "route_table_update"},
	    {timer_getfd(&ctx->timers.t_update), rip_t_update_expired, ctx, "t_update"},
	    {timer_getfd(&ctx->timers.t_triggered_lock), rip_t_triggered_lock_expired,
	     &ctx->timers.t_triggered_lock, "t_triggered_lock"},
	    {timer_getfd(&ctx->timers.t_timeout), rip_t_timeout_expired, ctx, "t_timeout"},
	    {timer_getfd(&ctx->timers.t_gc), rip_t_gc_expired, ctx, "t_gc"},
	    {timer_getfd(&ctx->timers.t_request_warmup), rip_t_request_warmup_expired, ctx,
	     "t_request_warmup"}};

	if (event_dispatcher_register_many(ed, events, ARRAY_LEN(events))) {
		return 1;
	}

	return 0;
}

static int rip_handle_events(struct rip_context *rip_ctx)
{
	LOG_INFO("Waiting for events...");

	while (1) {
		if (event_dispatcher_poll_and_dispatch(rip_ctx->event_dispatcher)) {
			return 1;
		}

		if (check_route_changed(rip_ctx)) {
			return 0;
		}
	}

	return 0;
}

static int add_advertised_network_to_db(struct rip_db			*db,
					const struct advertised_network *adv_network)
{
	struct rip_route_description desc;
	MEMSET_ZERO(&desc);

	desc.if_index = if_nametoindex(adv_network->dev);
	if (desc.if_index == 0) {
		LOG_ERR("if_nametoindex failed, dev: %s, errno %s", adv_network->dev,
			strerror(errno));
		return 1;
	}
	desc.in_routing_table = false;
	desc.is_local	      = true;

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
	entry->route_tag	 = 0; // arbitrary number
	entry->metric		 = 1;

	rip2_entry_hton(entry);
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

static int rip_init_timers(struct rip_timers *timers)
{
	if (timer_init(&timers->t_update) ||
	    timer_start_oneshot(&timers->t_update, create_t_update_time())) {
		LOG_ERR("t_update");
		return 1;
	}

	if (timer_init(&timers->t_triggered_lock)) {
		LOG_ERR("t_triggered_lock");
		return 1;
	}

	if (timer_init(&timers->t_request_warmup) ||
	    timer_start_oneshot(&timers->t_request_warmup, get_random_float(0.5, 1.0))) {
		LOG_ERR("t_request_warmup");
		return 1;
	}

	if (timer_init(&timers->t_timeout) ||
	    timer_start_oneshot(&timers->t_timeout, TIMEOUT_PERIOD_TIMER)) {
		LOG_ERR("t_timeout");
		return 1;
	}

	if (timer_init(&timers->t_gc)) {
		LOG_ERR("t_gc");
		return 1;
	}

	return 0;
}

int rip_begin(struct rip_context *ctx)
{
	LOG_INFO("%s", __func__);

	srand(time(NULL));

	if (rip_read_config(RIP_CONFIG_FILENAME, &ctx->config)) {
		LOG_ERR("failed to read configuration");
		return 1;
	}
	rip_configuration_print(&ctx->config);

	if (rip_create_sockets(&ctx->config, &ctx->rip_ifcs)) {
		return 1;
	}

	if (rip_setup_sockets(&ctx->rip_ifcs)) {
		LOG_ERR("failed to setup_resources");
		return 1;
	}
	rip_print_sockets(&ctx->rip_ifcs);

	ctx->route_mngr = rip_route_init();
	if (!ctx->route_mngr) {
		return 1;
	}

	ctx->rip_db = rip_db_init();
	if (!ctx->rip_db) {
		return 1;
	}

	if (add_advertised_networks_to_db(ctx->rip_db, &ctx->config)) {
		return 1;
	}

	struct r_ipc_cmd_handler handlers[] = {
	    {.cmd = dump_libnl_route_table, .data = ctx->route_mngr, .cb = rip_route_sprintf_table},
	    {.cmd = dump_rip_routes, .data = ctx->rip_db, .cb = rip_db_dump}};

	ctx->ipc_mngr = rip_ipc_alloc_init(handlers, ARRAY_LEN(handlers));
	if (!ctx->ipc_mngr) {
		return 1;
	}

	rip_init_timers(&ctx->timers);

	if (init_event_dispatcher(ctx)) {
		LOG_ERR("init_event_dispatcher");
		return 1;
	}

	return rip_handle_events(ctx);
}

static void rip_clear_routing_table(struct rip_db *db, struct rip_route_mngr *route_mngr)
{
	LOG_INFO("%s", __func__);

	struct rip_db_iter		    db_iter = {0};
	const struct rip_route_description *route;
	while (rip_db_iter_const(db, rip_db_ok, &db_iter, &route)) {
		// skip local addresses
		if (route->is_local) {
			continue;
		}

		if (rip_route_delete_route(route_mngr, route)) {
			LOG_ERR("failed to delete route");
			rip_route_description_print(route, stdout);
		}
	}
}

void rip_cleanup(struct rip_context *ctx)
{
	LOG_INFO("%s", __func__);

	rip_send_advertisement_shutdown(ctx->rip_db, &ctx->rip_ifcs);
	rip_clear_routing_table(ctx->rip_db, ctx->route_mngr);
}
