#ifndef RIP_COMMON_H
#define RIP_COMMON_H

#include "rip_messages.h"
#include <assert.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>

struct rip_route_description {
	struct rip2_entry entry;
	uint32_t next_hop_if_index;
};
//make sure it's padded
static_assert(sizeof(struct rip_route_description) == 24, "");

int get_prefix_len(struct in_addr subnet_mask);
void rip_route_description_print(const struct rip_route_description *descr,
				 FILE *file);

#endif
