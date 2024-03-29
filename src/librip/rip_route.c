#include "rip_route.h"
#include "utils/event.h"
#include "utils/logging.h"
#include "rip_common.h"
#include "rip_ipc.h"
#include "utils/utils.h"
#include <arpa/inet.h>
#include <assert.h>
#include <linux/rtnetlink.h>
#include <netinet/in.h>
#include <netlink/cache.h>
#include <netlink/errno.h>
#include <netlink/handlers.h>
#include <netlink/netlink.h>
#include <netlink/route/link.h>
#include <netlink/route/nexthop.h>
#include <netlink/route/route.h>
#include <netlink/socket.h>
#include <stdint.h>
#include <stdlib.h>

struct rip_route_mngr {
	// Socket for manipulating routing table
	// Not reusing mngr sock as it adviced
	// by the libnl to only handle kernel notifications
	struct nl_sock *requests_sock;

	//owned by mngr
	struct nl_cache *route_cache;

	//owned by mngr
	struct nl_cache *link_cache;	

	struct nl_cache_mngr *mngr;
};

static void dump_caches(struct rip_route_mngr *rr, struct nl_dump_params *dump_params)
{
	struct rtnl_route *filter = rtnl_route_alloc();
	if (!filter) {
		LOG_ERR("rtnl_route_alloc");
		return;
	}
	rtnl_route_set_protocol(filter, RTPROT_RIP);
	nl_cache_dump_filter(rr->route_cache, dump_params, OBJ_CAST(filter));
	free(filter);
}

struct rip_route_mngr *rip_route_init(void)
{
	struct rip_route_mngr *rr = NULL;
	int ec;

	rr = CALLOC(sizeof(struct rip_route_mngr));

	if (!rr) {
		LOG_ERR("%s: malloc", __func__);
		goto error;
	}

	rr->requests_sock = nl_socket_alloc();
	if (!rr->requests_sock) {
		LOG_ERR("nl_socket_alloc");
		goto error;
	}
	if ((ec = nl_connect(rr->requests_sock, NETLINK_ROUTE)) < 0) {
		LOG_ERR("nl_connect: %s\n", nl_geterror(ec));
		goto error;
	}

	ec = nl_cache_mngr_alloc(NULL, NETLINK_ROUTE, NL_AUTO_PROVIDE, &rr->mngr);
	if (ec < 0) {
		LOG_ERR("nl_cache_mngr_alloc: %s\n", nl_geterror(ec));
		goto error;
	}

	if ((ec = nl_cache_mngr_add(rr->mngr, "route/link", NULL, NULL, &rr->link_cache)) < 0) {
		LOG_ERR("nl_cache_mngr_add_cache route/link: %s\n", nl_geterror(ec));
		goto error;
	}

	if ((ec = nl_cache_mngr_add(rr->mngr, "route/route", NULL, NULL, &rr->route_cache)) < 0) {
		LOG_ERR("nl_cache_mngr_add_cache route_cache: %s\n", nl_geterror(ec));
		goto error;
	}

	return rr;
error:
	rip_route_free(rr);
	return NULL;
}

void rip_route_free(struct rip_route_mngr *rr)
{
	if (!rr) {
		return;
	}
	nl_cache_mngr_free(rr->mngr);
	nl_socket_free(rr->requests_sock);
	free(rr);
}

int rip_route_getfd(struct rip_route_mngr *rr) { return nl_cache_mngr_get_fd(rr->mngr); }

int rip_route_handle_event(const struct event *event)
{
	struct rip_route_mngr *rr = event->arg;

	int err;
	if ((err = nl_cache_mngr_data_ready(rr->mngr)) < 0) {
		LOG_ERR("nl_cache_mngr_data_ready failed: %s", nl_geterror(err));
		PANIC();
	}

	return 0;
}

static void rip_fill_next_hop(struct rtnl_nexthop *nh, struct nl_addr *nh_addr, int nh_if_index)
{
	rtnl_route_nh_set_gateway(nh, nh_addr);
	rtnl_route_nh_set_ifindex(nh, nh_if_index);
	rtnl_route_nh_set_weight(nh, 50); // TODO: find out what is it for
}

static int rip_fill_route(struct rtnl_route *route, struct nl_addr *dest, struct rtnl_nexthop *next_hop, uint8_t metric)
{
	int ec = 0;

	if ((ec = rtnl_route_set_family(route, AF_INET) < 0)) {
		LOG_ERR("rtnl_route_set_family: %s", nl_geterror(ec));
		return 1;
	}
	if ((ec = rtnl_route_set_type(route, RTN_UNICAST) < 0)) {
		LOG_ERR("rtnl_route_set_type: %s", nl_geterror(ec));
		return 1;
	}
	rtnl_route_set_table(route, RT_TABLE_MAIN);
	rtnl_route_set_protocol(route, RTPROT_RIP);
	rtnl_route_set_priority(route, 20 + metric);
	rtnl_route_add_nexthop(route, next_hop);
	if ((ec = rtnl_route_set_dst(route, dest) < 0)) {
		LOG_ERR("rtnl_route_set_dst: %s", nl_geterror(ec));
		return 1;
	}

	return 0;
}

static int rip_fill_nl_addr(struct in_addr in_addr, uint8_t prefix_len, struct nl_addr *nl_addr)
{
	int ec;

	nl_addr_set_family(nl_addr, AF_INET);
	if ((ec = nl_addr_set_binary_addr(nl_addr, &in_addr, sizeof(in_addr)) < 0)) {
		LOG_ERR("nl_addr_set_binary_addr: %s", nl_geterror(ec));
		return 1;
	}

	nl_addr_set_prefixlen(nl_addr, prefix_len);
	return 0;
}

static int rip_rtnl_route_create_alloc(struct rtnl_route **route, struct nl_addr **dest_nl,
				       struct nl_addr **nexthop_nl_adr, struct rtnl_nexthop **nexthop_nl)
{
	if (!(*route = rtnl_route_alloc())) {
		LOG_ERR("rtnl_route_alloc");
		goto alloc_failed;
	}

	if (!(*nexthop_nl = rtnl_route_nh_alloc())) {
		LOG_ERR("rtnl_route_nh_alloc");
		goto alloc_failed;
	}

	if (!(*dest_nl = nl_addr_alloc(sizeof(struct in_addr)))) {
		LOG_ERR("nl_addr_alloc");
		goto alloc_failed;
	}

	if (!(*nexthop_nl_adr = nl_addr_alloc(sizeof(struct in_addr)))) {
		LOG_ERR("nl_addr_alloc");
		goto alloc_failed;
	}
	return 0;
alloc_failed:
	nl_addr_put(*dest_nl);
	nl_addr_put(*nexthop_nl_adr);
	rtnl_route_nh_free(*nexthop_nl);
	rtnl_route_put(*route);
	return 1;
}

static struct rtnl_route *rip_rtnl_route_create(const struct rip_route_description *rd)
{
	struct rtnl_route *route	= NULL;
	struct nl_addr *dest_nl		= NULL;
	struct nl_addr *nexthop_nl_addr = NULL;
	struct rtnl_nexthop *nexthop_nl = NULL;

	if (rip_rtnl_route_create_alloc(&route, &dest_nl, &nexthop_nl_addr, &nexthop_nl) > 0) {
		LOG_ERR("rip_rtnl_route_create_alloc");
		return NULL;
	}

	const struct rip2_entry *entry = &rd->entry;
	if (rip_fill_nl_addr(entry->ip_address, get_prefix_len(entry->subnet_mask), dest_nl) > 0) {
		LOG_ERR("rip_fill_nl_addr");
		goto fill_error;
	}

	if (rip_fill_nl_addr(entry->next_hop, 32, nexthop_nl_addr) > 0) {
		LOG_ERR("rip_fill_nl_addr");
		goto fill_error;
	}

	rip_fill_next_hop(nexthop_nl, nexthop_nl_addr, rd->if_index);
	if (rip_fill_route(route, dest_nl, nexthop_nl, entry->metric) > 0) {
		LOG_ERR("rip_fill_route");
		goto fill_error;
	}

	nl_addr_put(dest_nl);
	nl_addr_put(nexthop_nl_addr);
	return route;

fill_error:
	nl_addr_put(dest_nl);
	nl_addr_put(nexthop_nl_addr);
	rtnl_route_nh_free(nexthop_nl);
	rtnl_route_put(route);

	return NULL;
}

struct rtnl_route *rip_route_entry_create(const struct rip_route_description *route_description)
{
	LOG_INFO("%s", __func__);
	struct rtnl_route *route = NULL;

	if (!(route = rip_rtnl_route_create(route_description))) {
		LOG_ERR("rip_rtnl_route_create");
		return NULL;
	}

	struct nl_dump_params dump_params = {
	    .dp_fd   = stdout,
	    .dp_type = NL_DUMP_LINE,
	};

	nl_object_dump(OBJ_CAST(route), &dump_params);

	return route;
}

int rip_route_add_route(struct rip_route_mngr *rr, const struct rip_route_description *route_entry_input)
{
	int ec			 = 0;
	int ret			 = 0;
	struct rtnl_route *route = NULL;

	if (!(route = rip_rtnl_route_create(route_entry_input))) {
		return 1;
	}

	if ((ec = rtnl_route_add(rr->requests_sock, route, NLM_F_EXCL)) < 0) {
		LOG_ERR("rtnl_route_add: %s", nl_geterror(ec));
		ret = 1;
	}

	rtnl_route_put(route);
	return ret;
}

int rip_route_delete_route(struct rip_route_mngr *rr, const struct rip_route_description *route_entry_input)
{
	int ec			 = 0;
	int ret			 = 0;
	struct rtnl_route *route = NULL;

	if (!(route = rip_rtnl_route_create(route_entry_input))) {
		return 1;
	}

	if ((ec = rtnl_route_delete(rr->requests_sock, route, 0)) < 0) {
		LOG_ERR("rtnl_route_add: %s", nl_geterror(ec));
		ret = 1;
	}

	rtnl_route_put(route);
	return ret;
}

void rip_route_print_table(struct rip_route_mngr *ri) { rip_route_sprintf_table(stdout, ri); }

enum r_cmd_status rip_route_sprintf_table(FILE *file, void *data)
{
	assert(data);

	struct rip_route_mngr *mngr	  = data;
	struct nl_dump_params dump_params = {
	    .dp_fd   = file,
	    .dp_type = NL_DUMP_LINE,
	};

	dump_caches(mngr, &dump_params);
	return r_cmd_status_success;
}
