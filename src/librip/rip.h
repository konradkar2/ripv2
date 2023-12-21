#ifndef RIP_H
#define RIP_H

#include "rip_db.h"
#include "rip_ipc.h"
#include "rip_route.h"
#include "utils/config/parse_rip_config.h"
#include "utils/event_dispatcher.h"
#include "utils/timer.h"
#include <net/if.h>
#include <stddef.h>

struct rip_context {
	struct rip_configuration config;
	struct rip_ifc *rip_ifcs;
	size_t rip_ifcs_n;

	struct timer t_update;
	struct rip_route_mngr *route_mngr;
	struct rip_ipc *ipc_mngr;
	struct rip_db rip_db;
	struct event_dispatcher event_dispatcher;
	enum rip_state state;
};

int rip_begin(struct rip_context *rip_ctx);

#endif
