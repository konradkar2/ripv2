#ifndef RIP_DATABASE_H
#define RIP_DATABASE_H

#include <netinet/in.h>
#include <stdint.h>

typedef struct rip_network {
	struct in_addr ip_address;
	struct in_addr subnet_mask;
} rip_network;

typedef struct rip_db_entry {
	rip_network network;
	uint32_t metric;
	struct in_addr next_hop;
    int if_index_orig;
    int timer_fd;
} rip_db_entry;

#endif