#ifndef RIP_ROUTE_H
#define RIP_ROUTE_H

struct rip_route;

struct rip_route *rip_route_alloc_init(void);
void rip_route_free(struct rip_route *);

void rip_route_print_table(struct rip_route *);

#endif