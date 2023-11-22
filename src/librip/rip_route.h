#ifndef RIP_ROUTE_H
#define RIP_ROUTE_H

#include "rip_ipc.h"
#include <netinet/in.h>
#include "rip_common.h"

struct rip_route_mngr;

struct rip_route_mngr *rip_route_alloc_init(void);
void rip_route_free(struct rip_route_mngr *);

// kernel communication
int rip_route_getfd(struct rip_route_mngr *);
int rip_route_handle_event(const struct rip_event *);

// routing table manipulation
int rip_route_add_route(struct rip_route_mngr *, const struct rip_route_description * route_entry_input);
int rip_route_delete_route(struct rip_route_mngr *, const struct rip_route_description * route_entry_input);

/// helpers
void rip_route_print_table(struct rip_route_mngr *);
enum r_cmd_status rip_route_sprintf_table(FILE * file, void *data);

#endif
