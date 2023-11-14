#ifndef RIP_DATABASE_H
#define RIP_DATABASE_H

#include "rip_common.h"
#include <netinet/in.h>
#include <stdbool.h>
#include <stdint.h>
#include <utils/vector.h>

struct rip_db {
	struct vector *added_routes;
};

int rip_db_init(struct rip_db *);
void rip_db_destroy(struct rip_db *);

bool rip_db_add(struct rip_db *, struct rip_route_description *entry);
bool rip_db_contains(struct rip_db *, struct rip_route_description *entry);
bool rip_db_remove(struct rip_db *, struct rip_route_description *entry);

//void rip_db_dump(char *resp_buffer, size_t buffer_size, void *data);

#endif
