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
void rip_route_handle_netlink_io(struct rip_route_mngr *);

typedef void rip_route_entry;
rip_route_entry *rip_route_entry_create(const struct rip_route_description * route_entry_input);
void rip_route_entry_free(rip_route_entry *);

// routing table manipulation
int rip_route_add_route(struct rip_route_mngr *, rip_route_entry *entry);

/// helpers
void rip_route_print_table(struct rip_route_mngr *);
int rip_route_sprintf_table(char *resp_buffer, size_t buffer_size, void *data);

#endif
