#ifndef RIP_COMMON_H
#define RIP_COMMON_H

#include <netinet/in.h>
#include <stdint.h>

struct rip_route_description {
	struct in_addr dest_addr;
	int next_hop_if_index;
	struct in_addr nexthop_addr;
	uint8_t dest_prefix_len;
	uint8_t metric;
};

#endif
