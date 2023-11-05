#ifndef RIP_COMMON_H
#define RIP_COMMON_H

#include "rip_messages.h"
#include <netinet/in.h>
#include <stdint.h>

struct rip_route_description {
	struct rip2_entry entry;
	int next_hop_if_index;
};

#endif
