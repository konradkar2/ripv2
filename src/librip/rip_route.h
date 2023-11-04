#ifndef RIP_ROUTE_H
#define RIP_ROUTE_H

#include "rip_ipc.h"
#include <netinet/in.h>

struct rip_route;

struct rip_route *rip_route_alloc_init(void);
void rip_route_free(struct rip_route *);

// kernel communication
int rip_route_getfd(struct rip_route *);
void rip_route_handle_netlink_io(struct rip_route *);

typedef void rip_route_entry;
rip_route_entry *rip_route_entry_create(struct in_addr dest, int dest_prefix,
					int next_hop_if_index,
					struct in_addr next_hop);
void rip_route_entry_free(rip_route_entry*);

// routing table manipulation
int rip_route_add_route(struct rip_route *, rip_route_entry *entry);

/// helpers
void rip_route_print_table(struct rip_route *);
int rip_route_sprintf_table(char *resp_buffer, size_t buffer_size, void *data);

#endif
