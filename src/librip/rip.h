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
	struct rip_ifc_vec	 rip_ifcs;
	
	struct rip_timers {
		struct timer t_update;
		struct timer t_triggered_lock;

		// warmup timer to send multicast request
		// this is to not miss any request when
		// starting rip processes at the same time
		struct timer t_request_warmup;

		// global timer instead of unique one per route
		//  triggered every 30seconds
		struct timer t_timeout;
	} timers;

	struct rip_route_mngr	*route_mngr;
	struct rip_ipc		*ipc_mngr;
	struct rip_db		*rip_db;
	struct event_dispatcher *event_dispatcher;
};

int  rip_begin(struct rip_context *rip_ctx);
void rip_cleanup(struct rip_context *rip_ctx);

#endif
