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
	struct nl_cache * link_cache;
	struct nl_cache_mngr *mngr;
};

static void dump_caches(struct rip_route *rr,
			struct nl_dump_params *dump_params)
{
	struct rtnl_route *route_filter = rtnl_route_alloc();
	if (!route_filter) {
		LOG_ERR("rtnl_route_alloc");
		return;
	}

	nl_cache_dump_filter(rr->route_cache, dump_params, OBJ_CAST(route_filter));
	rtnl_route_put(route_filter);
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

	if ((err = rtnl_route_alloc_cache(rr->sock, AF_INET, 0,
					  &rr->route_cache)) < 0) {
		LOG_ERR("rtnl_route_alloc_cache: %s\n", nl_geterror(err));
		goto error;
	}

	if((err = rtnl_link_alloc_cache(rr->sock, AF_INET, &rr->link_cache))){
		LOG_ERR("rtnl_link_alloc_cache: %s\n", nl_geterror(err));
		goto error;
	}

	if ((err = nl_cache_mngr_add_cache(rr->mngr, rr->route_cache, NULL,
					   NULL)) < 0) {
		LOG_ERR("nl_cache_mngr_add_cache route_cache: %s\n",
			nl_geterror(err));
		goto error;
	}

	if ((err = nl_cache_mngr_add_cache(rr->mngr, rr->link_cache, NULL,
					   NULL)) < 0) {
		LOG_ERR("nl_cache_mngr_add_cache route_cache: %s\n",
			nl_geterror(err));
		goto error;
	}

	//needed for more detailed dumps
	//TODO: remove or enabled under flag
	nl_cache_mngt_provide(rr->route_cache);
	nl_cache_mngt_provide(rr->link_cache);

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
	nl_cache_free(rr->link_cache);
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
	nl_addr_put(nh_addr);
	rtnl_route_nh_set_ifindex(nh, nh_if_index);
	rtnl_route_nh_set_weight(nh, 50); // TODO: find out what is it for
}

static int rip_fill_route(struct rtnl_route *route, struct nl_addr *dest,
			  struct rtnl_nexthop *next_hop)
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
	rtnl_route_set_priority(route, 20);
	rtnl_route_add_nexthop(route, next_hop);
	if ((ec = rtnl_route_set_dst(route, dest) < 0)) {
		LOG_ERR("rtnl_route_set_dst: %s", nl_geterror(ec));
		return 1;
	}
	nl_addr_put(dest);

	return 0;
}

static int rip_fill_nl_addr(struct in_addr in_addr, int prefix_len,
			    struct nl_addr *nl_addr)
{
	int ec;

	nl_addr_set_family(nl_addr, AF_INET);
	if ((ec = nl_addr_set_binary_addr(nl_addr, &in_addr, sizeof(in_addr)) <
		  0)) {
		LOG_ERR("nl_addr_set_binary_addr: %s", nl_geterror(ec));
		return 1;
	}

	nl_addr_set_prefixlen(nl_addr, prefix_len);
	return 0;
}

static int rip_rtnl_route_create_alloc(struct rtnl_route **route,
				       struct nl_addr **dest_nl,
				       struct nl_addr **nexthop_nl_adr,
				       struct rtnl_nexthop **nexthop_nl)
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
	LOG_ERR("alloc_failed");
	nl_addr_put(*dest_nl);
	nl_addr_put(*nexthop_nl_adr);
	rtnl_route_nh_free(*nexthop_nl);
	rtnl_route_put(*route);
	return 1;
}

struct rtnl_route *rip_rtnl_route_create(struct in_addr dest_in_addr,
					 int dest_prefix, int nexthop_if_index,
					 struct in_addr nexthop_in_addr)
{
	struct rtnl_route *route	= NULL;
	struct nl_addr *dest_nl		= NULL;
	struct nl_addr *nexthop_nl_addr = NULL;
	struct rtnl_nexthop *nexthop_nl = NULL;

	if (rip_rtnl_route_create_alloc(&route, &dest_nl, &nexthop_nl_addr,
					&nexthop_nl) > 0) {
		LOG_ERR("rip_rtnl_route_create_alloc");
		return NULL;
	}

	if (rip_fill_nl_addr(dest_in_addr, dest_prefix, dest_nl) > 0) {
		LOG_ERR("rip_fill_nl_addr");
		goto fill_error;
	}

	if (rip_fill_nl_addr(nexthop_in_addr, 32, nexthop_nl_addr) > 0) {
		LOG_ERR("rip_fill_nl_addr");
		goto fill_error;
	}

	rip_fill_next_hop(nexthop_nl, nexthop_nl_addr, nexthop_if_index);
	if (rip_fill_route(route, dest_nl, nexthop_nl) > 0) {
		LOG_ERR("rip_fill_route");
		goto fill_error;
	}

	LOG_INFO("return");
	return route;

fill_error:
	LOG_ERR("fill_error");
	nl_addr_put(dest_nl);
	nl_addr_put(nexthop_nl_addr);
	rtnl_route_nh_free(nexthop_nl);
	rtnl_route_put(route);

	return NULL;
}

rip_route_entry *rip_route_entry_create(struct in_addr dest_in_addr,
					int dest_prefix, int nexthop_if_index,
					struct in_addr nexthop_in_addr)
{
	LOG_INFO("%s", __func__);
	struct rtnl_route *route = NULL;

	if (!(route =
		  rip_rtnl_route_create(dest_in_addr, dest_prefix,
					nexthop_if_index, nexthop_in_addr))) {
		LOG_ERR("rip_rtnl_route_create");
		return NULL;
	}
	LOG_INFO("%s dump ", __func__);
	struct nl_dump_params dump_params = {
	    .dp_fd   = stdout,
	    .dp_type = NL_DUMP_LINE,
	};

	nl_object_dump(OBJ_CAST(route), &dump_params);

	return route;
}

void rip_route_entry_free(rip_route_entry *entry)
{
	struct rtnl_route *route = entry;
	rtnl_route_put(route);
}

int rip_route_add_route(struct rip_route *rr, rip_route_entry *entry)
{
	LOG_INFO("%s", __func__);
	int ec;
	struct rtnl_route *route = entry;
	if ((ec = rtnl_route_add(rr->sock, route, NLM_F_EXCL)) < 0) {
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
