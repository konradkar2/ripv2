#include "rip_route.h"
#include "logging.h"
#include "netlink/cache.h"
#include "netlink/errno.h"
#include "netlink/netlink.h"
#include <assert.h>
#include <netlink/handlers.h>
#include <netlink/route/link.h>
#include <netlink/route/route.h>
#include <netlink/socket.h>
#include <stdlib.h>

struct rip_route {
	struct nl_sock *sock;
	struct rip_route *rr;
	struct nl_cache *link_cache;
	struct nl_cache *route_cache;
	struct nl_cache_mngr *mngr;
};

static void dump_caches(struct rip_route *rr)
{
	struct rtnl_route *route_dump_filter;
	struct rtnl_link *link_dump_filter;
	struct nl_dump_params dump_params = {
	    .dp_fd   = stdout,
	    .dp_type = NL_DUMP_LINE,
	};

	LOG_INFO("rtnl_route_alloc:");
	route_dump_filter = rtnl_route_alloc();
	if (!route_dump_filter) {
		LOG_ERR("rtnl_route_alloc");
		return;
	}

	LOG_INFO("rtnl_link_alloc:");
	link_dump_filter = rtnl_link_alloc();
	if (!link_dump_filter) {
		LOG_ERR("rtnl_link_alloc");
		return;
	}

	LOG_INFO("nl_cache_dump_filter link:");
	nl_cache_dump_filter(rr->link_cache, &dump_params,
			     OBJ_CAST(link_dump_filter));

	LOG_INFO("nl_cache_dump_filter route:");
	nl_cache_dump_filter(rr->route_cache, &dump_params,
			     OBJ_CAST(route_dump_filter));

	nl_object_free(OBJ_CAST(link_dump_filter));
	nl_object_free(OBJ_CAST(route_dump_filter));
}

struct rip_route *rip_route_alloc_init(void)
{
	struct rip_route *rr;

	int err;

	rr = malloc(sizeof(struct rip_route));
	assert(rr && "buy more ram lol");

	rr->sock = nl_socket_alloc();
	assert(rr->sock && "buy more ram lol");

	err = nl_cache_mngr_alloc(rr->sock, NETLINK_ROUTE, 0, &rr->mngr);
	if (err < 0) {
		LOG_ERR("nl_cache_mngr_alloc: %s\n", nl_geterror(err));
		goto error;
	}

	LOG_INFO("rtnl_link_alloc_cache:");
	if ((err = rtnl_link_alloc_cache(rr->sock, AF_UNSPEC,
					 &rr->link_cache)) < 0) {
		LOG_ERR("rtnl_link_alloc_cache_flags: %s\n", nl_geterror(err));
		goto error;
	}

	LOG_INFO("rtnl_route_alloc_cache:");
	if ((err = rtnl_route_alloc_cache(rr->sock, AF_UNSPEC, 0,
					  &rr->route_cache)) < 0) {
		LOG_ERR("rtnl_route_alloc_cache: %s\n", nl_geterror(err));
		goto error;
	}
	dump_caches(rr);

	LOG_INFO("nl_cache_mngr_add_cache:");
	if ((err = nl_cache_mngr_add_cache(rr->mngr, rr->link_cache, NULL,
					   NULL)) < 0) {
		LOG_ERR("nl_cache_mngr_add_cache link_cache: %s\n",
			nl_geterror(err));
		goto error;
	}

	LOG_INFO("nl_cache_mngr_add_cache:");
	if ((err = nl_cache_mngr_add_cache(rr->mngr, rr->route_cache, NULL,
					   NULL)) < 0) {
		LOG_ERR("nl_cache_mngr_add_cache route_cache: %s\n",
			nl_geterror(err));
		goto error;
	}

	return rr;
error:
	LOG_ERR("goto error");
	rip_route_free(rr);
	return NULL;
}

int rip_route_getfd(struct rip_route *rr)
{
	return nl_cache_mngr_get_fd(rr->mngr);
}

void rip_route_update(struct rip_route *rr)
{
	nl_cache_mngr_data_ready(rr->mngr);
	dump_caches(rr);
}

void rip_route_free(struct rip_route *rr)
{
	if (rr) {
		// nl_socket_free(rr->nl_sock);
		// free(rr);
	}
}

void rip_route_print_table(struct rip_route *rr)
{
	(void)rr;
	LOG_INFO("%s", __func__);
}