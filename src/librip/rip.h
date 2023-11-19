#ifndef RIP_H
#define RIP_H

#include "config/parse_rip_config.h"
#include "rip_db.h"
#include "rip_if.h"
#include "rip_ipc.h"
#include "rip_route.h"
#include <stddef.h>

#define MAX_RIP_IFS 16

typedef struct {
	int fd;
	int interval;
} rip_timer;

struct rip_context {
	struct rip_configuration config;
	rip_if_entry rip_ifs[MAX_RIP_IFS];
	size_t rip_ifs_count;
	rip_timer t_update;
	rip_timer t_timeout;
	rip_timer t_garbage_collection;
	struct rip_route_mngr *route_mngr;
	struct rip_ipc *ipc_mngr;
	struct rip_db rip_db;
};

int rip_begin(struct rip_context *rip_ctx);
int rip_if_entry_find_by_fd(const struct rip_context *rip_ctx, int fd, size_t *rip_ifs_idx);

#endif
