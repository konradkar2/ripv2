#ifndef RIP_H
#define RIP_H

#include "rip_if.h"
#include <stddef.h>

#define MAX_RIP_IFS 16

typedef struct rip_context {
	rip_if_entry rip_ifs[MAX_RIP_IFS];
	size_t rip_ifs_count;
} rip_context;

int rip_begin(rip_context *rip_ctx);

//TODO: move to different file -> test!
int rip_if_entry_find_by_fd(const rip_context *rip_ctx, const int fd,
			    size_t *index_found);

#endif