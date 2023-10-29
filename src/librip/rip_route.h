#ifndef RIP_ROUTE_H
#define RIP_ROUTE_H

#include "rip_ipc.h"

struct rip_route;

struct rip_route *rip_route_alloc_init(void);
void rip_route_free(struct rip_route *);

//kernel communication
int rip_route_getfd(struct rip_route *);
void rip_route_handle_netlink_io(struct rip_route *);

//routign table manipulation
int rip_route_add_route(struct rip_route*);


/// helpers
void rip_route_print_table(struct rip_route *);
int rip_route_sprintf_table(char *resp_buffer, size_t buffer_size, void *data);

#endif
