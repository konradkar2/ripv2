#ifndef RIP_DATABASE_H
#define RIP_DATABASE_H

#include "rip_common.h"
#include "rip_ipc.h"
#include <netinet/in.h>
#include <stdbool.h>
#include <stdint.h>
#include <utils/hashmap.h>

struct rip_db {
	struct hashmap *added_routes;
};

int rip_db_init(struct rip_db *);
void rip_db_destroy(struct rip_db *);

int rip_db_add(struct rip_db *, struct rip_route_description *entry);

// perform lookup by network address
const struct rip_route_description *
rip_db_get(struct rip_db *, struct rip_route_description *entry);

int rip_db_remove(struct rip_db *, struct rip_route_description *entry);

enum r_cmd_status rip_db_dump(FILE *file, void *data);

#endif
