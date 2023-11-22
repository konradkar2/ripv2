#ifndef RIP_H
#define RIP_H

#include "utils/config/parse_rip_config.h"
#include "rip_db.h"
#include "rip_ipc.h"
#include "rip_route.h"
#include <net/if.h>
#include <stddef.h>
#include "utils/timer.h"

struct rip_ifc {
	int fd;
	char if_name[IF_NAMESIZE];
	int if_index;
};

struct rip_context {
	struct rip_configuration config;
	struct rip_ifc *rip_ifcs;
	size_t rip_ifcs_n;
	struct timer t_update;
	struct rip_route_mngr *route_mngr;
	struct rip_ipc *ipc_mngr;
	struct rip_db rip_db;
	enum rip_state state;
};

int rip_begin(struct rip_context *rip_ctx);

#endif
