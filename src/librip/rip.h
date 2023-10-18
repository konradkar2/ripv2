#ifndef RIP_H
#define RIP_H

#include "rip_if.h"
#include <stddef.h>

#define MAX_RIP_IFS 16

typedef struct {
	int fd;
	int interval;
} rip_timer;

typedef struct {
	rip_if_entry rip_ifs[MAX_RIP_IFS];
	size_t rip_ifs_count;
	rip_timer t_update;
	rip_timer t_timeout;
	rip_timer t_garbage_collection;
	struct rip_route *route_mngr;
} rip_context;

int rip_begin(rip_context *rip_ctx);

// TODO: move to different file -> test!
int rip_if_entry_find_by_fd(const rip_context *rip_ctx, int fd,
			    size_t *rip_ifs_idx);

#endif