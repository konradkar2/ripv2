#ifndef RIP_DATABASE_H
#define RIP_DATABASE_H

#include "rip_common.h"
#include <netinet/in.h>
#include <stdbool.h>
#include <stdint.h>

#define RIP_DB_LENGTH 500

struct rip_db_entry {
	struct rip_route_description route;
	// interface on which route has been received
	int if_index_origin;
};

struct rip_db {
	struct rip_db_entry added_routes[RIP_DB_LENGTH];
};

bool rip_db_contains(const struct rip_db_entry * entry);

#endif
