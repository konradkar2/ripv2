#include "rip_route.h"
#include "logging.h"
#include "netlink/object.h"
#include "utils.h"
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

struct rip_route {
	struct nl_sock *sock;
	struct nl_cache *route_cache;
	struct nl_cache_mngr *mngr;
};

static void dump_caches(struct rip_route *rr,
			struct nl_dump_params *dump_params)
{
	struct nl_object *route_filter = OBJ_CAST(rtnl_route_alloc());
	if (!route_filter) {
		LOG_ERR("rtnl_route_alloc");
		return;
	}

	nl_cache_dump_filter(rr->route_cache, dump_params, route_filter);
	nl_object_free(route_filter);
}

struct rip_route *rip_route_alloc_init(void)
{
	struct rip_route *rr;
	int err;

	rr = CALLOC(sizeof(struct rip_route));

	if (!rr) {
		LOG_ERR("%s: malloc", __func__);
		goto error;
	}

	rr->sock = nl_socket_alloc();
	if (!rr->sock) {
		LOG_ERR("nl_socket_alloc");
		goto error;
	}

	err = nl_cache_mngr_alloc(rr->sock, NETLINK_ROUTE, 0, &rr->mngr);
	if (err < 0) {
		LOG_ERR("nl_cache_mngr_alloc: %s\n", nl_geterror(err));
		goto error;
	}

	if ((err = rtnl_route_alloc_cache(rr->sock, AF_UNSPEC, 0,
					  &rr->route_cache)) < 0) {
		LOG_ERR("rtnl_route_alloc_cache: %s\n", nl_geterror(err));
		goto error;
	}

	if ((err = nl_cache_mngr_add_cache(rr->mngr, rr->route_cache, NULL,
					   NULL)) < 0) {
		LOG_ERR("nl_cache_mngr_add_cache route_cache: %s\n",
			nl_geterror(err));
		goto error;
	}

	return rr;
error:
	rip_route_free(rr);
	return NULL;
}

void rip_route_free(struct rip_route *rr)
{
	if (!rr) {
		return;
	}

	nl_cache_mngr_free(rr->mngr);
	nl_cache_free(rr->route_cache);
	nl_socket_free(rr->sock);
	free(rr);
}

int rip_route_getfd(struct rip_route *rr)
{
	return nl_cache_mngr_get_fd(rr->mngr);
}

void rip_route_handle_netlink_io(struct rip_route *rr)
{
	int err;
	if ((err = nl_cache_mngr_data_ready(rr->mngr)) < 0) {
		LOG_ERR("nl_cache_mngr_data_ready failed: %s",
			nl_geterror(err));
	}
}

static void rip_fill_next_hop(struct rtnl_nexthop *nh, struct nl_addr *nh_addr,
			      int nh_if_index)
{
	rtnl_route_nh_set_gateway(nh, nh_addr);
	rtnl_route_nh_set_ifindex(nh, nh_if_index);
	rtnl_route_nh_set_weight(nh, 30); // TODO: find out what is it for
}

static int rip_fill_route(struct rtnl_route *route, struct nl_addr *dest,
			  struct rtnl_nexthop *next_hop)
{
	int ec = 0;
	rtnl_route_set_table(route, RT_TABLE_MAIN);
	rtnl_route_set_protocol(route, RTPROT_RIP);
	rtnl_route_set_priority(route, 50);
	rtnl_route_add_nexthop(route, next_hop);
	if ((ec = rtnl_route_set_dst(route, dest) < 0)) {
		LOG_ERR("rtnl_route_set_dst: %s", nl_geterror(ec));
		return 1;
	}

	return 0;
}

static struct nl_addr *rip_create_nl_addr(struct in_addr addr, int prefix_len)
{
	struct nl_addr *ret = NULL;
	int ec;

	if (!(ret = nl_addr_alloc(sizeof(struct in_addr)))) {
		LOG_ERR("nl_addr_alloc");
		return NULL;
	}

	nl_addr_set_family(ret, AF_INET);
	if ((ec = nl_addr_set_binary_addr(ret, &addr, sizeof(addr)) < 0)) {
		LOG_ERR("nl_addr_set_binary_addr: %s", nl_geterror(ec));
		nl_addr_put(ret);
		return NULL;
	}

	nl_addr_set_prefixlen(ret, prefix_len);
	return ret;
}

struct rip_route_entry {
	struct rtnl_route *route;
};

struct rtnl_route *rip_rtnl_route_create(struct in_addr dest_in_addr,
					 int dest_prefix, int nexthop_if_index,
					 struct in_addr nexthop_in_addr)
{
	struct rtnl_route *route	= NULL;
	struct nl_addr *dest_nl		= NULL;
	struct nl_addr *nexthop_nl_adr	= NULL;
	struct rtnl_nexthop *nexthop_nl = NULL;

	if (!(route = rtnl_route_alloc())) {
		LOG_ERR("rtnl_route_alloc");
		goto alloc_failed;
	}

	if (!(nexthop_nl = rtnl_route_nh_alloc())) {
		LOG_ERR("rtnl_route_nh_alloc");
		goto alloc_failed;
	}

	if (!(dest_nl = rip_create_nl_addr(dest_in_addr, dest_prefix))) {
		LOG_ERR("rip_create_nl_addr");
		goto alloc_failed;
	}

	if (!(nexthop_nl_adr = rip_create_nl_addr(nexthop_in_addr, 32))) {
		LOG_ERR("rip_create_nl_addr");
		goto alloc_failed;
	}

	rip_fill_next_hop(nexthop_nl, nexthop_nl_adr, nexthop_if_index);
	if (rip_fill_route(route, dest_nl, nexthop_nl) > 0) {
		LOG_ERR("rip_fill_route");
		rtnl_route_put(route);
	}

	LOG_INFO("return");
	return route;

alloc_failed:
	LOG_ERR("alloc_failed");
	rtnl_route_put(route);
	nl_addr_put(dest_nl);
	nl_addr_put(nexthop_nl_adr);
	rtnl_route_nh_free(nexthop_nl);

	return NULL;
}

struct rip_route_entry *rip_route_entry_create(struct in_addr dest_in_addr,
					       int dest_prefix,
					       int nexthop_if_index,
					       struct in_addr nexthop_in_addr)
{
	struct rip_route_entry *entry = NULL;
	struct rtnl_route *route      = NULL;

	if (!(entry = CALLOC(sizeof(*entry)))) {
		LOG_ERR("entry alloc");
		return NULL;
	}

	if (!(route =
		  rip_rtnl_route_create(dest_in_addr, dest_prefix,
					nexthop_if_index, nexthop_in_addr))) {
		LOG_ERR("rip_rtnl_route_create");
		rip_route_entry_free(entry);
	}

	entry->route = route;
	return entry;
}

void rip_route_entry_free(struct rip_route_entry *entry)
{
	if (!entry) {
		return;
	}

	rtnl_route_put(entry->route);
	free(entry);
}

int rip_route_add_route(struct rip_route *rr,
			const struct rip_route_entry *entry)
{
	int ec;
	if ((ec = rtnl_route_add(rr->sock, entry->route, NLM_F_EXCL)) < 0) {
		LOG_ERR("rtnl_route_add: %s", nl_geterror(ec));
		return 1;
	}

	return 0;
}

void rip_route_print_table(struct rip_route *ri)
{
	struct nl_dump_params dump_params = {
	    .dp_fd   = stdout,
	    .dp_type = NL_DUMP_LINE,
	};

	dump_caches(ri, &dump_params);
}

int rip_route_sprintf_table(char *resp_buffer, size_t buffer_size, void *data)
{
	assert(data);

	struct rip_route *mngr		  = data;
	struct nl_dump_params dump_params = {
	    .dp_buf    = resp_buffer,
	    .dp_buflen = buffer_size,
	    .dp_type   = NL_DUMP_LINE,
	};

	dump_caches(mngr, &dump_params);

	return 0;
}
