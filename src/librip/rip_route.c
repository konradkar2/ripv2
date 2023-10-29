#include "rip_route.h"
#include "logging.h"
#include "utils.h"
#include <assert.h>
#include <netlink/cache.h>
#include <netlink/errno.h>
#include <netlink/handlers.h>
#include <netlink/netlink.h>
#include <netlink/route/link.h>
#include <netlink/route/route.h>
#include <netlink/socket.h>
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

int rip_route_add_route(struct rip_route *rr)
{
	(void)rr;
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
