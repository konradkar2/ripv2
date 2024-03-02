#ifndef RIP_DATABASE_H
#define RIP_DATABASE_H

#include "rip_common.h"
#include "rip_ipc.h"
#include <netinet/in.h>
#include <stdbool.h>
#include <utils/hashmap.h>

struct rip_db_iter {
	size_t updated_routes_iter;
	bool   updated_routes_iterated;
	size_t non_updated_routes_iter;
};

struct rip_db {
	// updated routes are these that got updated in last 60 seconds
	// non updated routes are the opposite
	// we split entire db to two parts to optimize timeout/garbage collection process
	struct hashmap *updated_routes;
	struct hashmap *non_updated_routes;
	bool		any_route_changed;
};

int  rip_db_init(struct rip_db *);
void rip_db_destroy(struct rip_db *);

int rip_db_add(struct rip_db *, struct rip_route_description *entry);

const struct rip_route_description *rip_db_get(struct rip_db *,
					       struct rip_route_description *entry);

int rip_db_remove(struct rip_db *, struct rip_route_description *entry);

// returns true if item was retrieved
bool rip_db_iter_all(struct rip_db *, struct rip_db_iter *iter,
		     const struct rip_route_description **desc);
bool rip_db_iter_non_updated(struct rip_db *, struct rip_db_iter *iter,
			     const struct rip_route_description **desc);

bool rip_db_any_route_changed(struct rip_db *);
void rip_db_mark_all_routes_as_unchanged(struct rip_db *db);

enum r_cmd_status rip_db_dump(FILE *file, void *data);

#endif
