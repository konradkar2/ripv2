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
	int xd;
};

struct rip_route *rip_route_alloc_init(void)
{
	struct nl_sock *sock;
	struct rip_route *rr;
	struct nl_cache *link_cache, *route_cache;
	struct rtnl_route *route_dump_filter;
	struct rtnl_link *link_dump_filter;
	struct nl_dump_params dump_params = {
	    .dp_fd   = stdout,
	    .dp_type = NL_DUMP_LINE,
	};
	int err;

	rr = malloc(sizeof(struct rip_route));
	assert(rr && "buy more ram lol");

	sock = nl_socket_alloc();
	assert(sock && "buy more ram lol");

	if ((err = nl_connect(sock, NETLINK_ROUTE) < 0)) {
		LOG_ERR("nl_connect: %s\n", nl_geterror(err));
		goto error;
	}

	LOG_INFO("rtnl_link_alloc_cache:");
	if ((err = rtnl_link_alloc_cache(sock, AF_UNSPEC, &link_cache)) < 0) {
		LOG_ERR("rtnl_link_alloc_cache_flags: %s\n", nl_geterror(err));
		goto error;
	}

	LOG_INFO("rtnl_route_alloc_cache:");
	LOG_ERR("rtnl_route_alloc_cache:");
	if ((err = rtnl_route_alloc_cache(sock, AF_UNSPEC, 0, &route_cache)) <
	    0) {
		LOG_ERR("rtnl_route_alloc_cache: %s\n", nl_geterror(err));
		goto error;
	}

	LOG_INFO("rtnl_route_alloc:");
	route_dump_filter = rtnl_route_alloc();
	if (!route_dump_filter) {
		LOG_ERR("rtnl_route_alloc");
		goto error;
	}

	LOG_INFO("rtnl_link_alloc:");
	link_dump_filter = rtnl_link_alloc();
	if (!link_dump_filter) {
		LOG_ERR("rtnl_link_alloc");
		goto error;
	}

	LOG_INFO("nl_cache_dump_filter link:");
	nl_cache_dump_filter(link_cache, &dump_params,
			     OBJ_CAST(link_dump_filter));

	LOG_INFO("nl_cache_dump_filter route:");
	nl_cache_dump_filter(route_cache, &dump_params,
			     OBJ_CAST(route_dump_filter));

	return rr;
error:
	LOG_ERR("goto error");
	rip_route_free(rr);
	return NULL;
}

/*
struct nl_sock *sock;
struct nl_cache *link_cache, *route_cache;
struct rtnl_route *route;
struct nl_dump_params params = {
    .dp_fd   = stdout,
    .dp_type = NL_DUMP_LINE,
};
int print_cache = 0;

sock = nl_cli_alloc_socket();
nl_cli_connect(sock, NETLINK_ROUTE);
link_cache = nl_cli_link_alloc_cache(sock);
route	   = nl_cli_route_alloc();
*/

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