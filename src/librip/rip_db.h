#ifndef RIP_DATABASE_H
#define RIP_DATABASE_H

#include "rip_common.h"
#include "rip_ipc.h"
#include "utils/utils.h"
#include <netinet/in.h>
#include <stdbool.h>
#include <utils/hashmap.h>

enum rip_db_type { rip_db_all, rip_db_ok, rip_db_garbage };

struct rip_db_iter {
	size_t ok_routes_iter;
	bool   ok_routes_iterated;
	size_t garbage_routes_iter;
};

struct rip_db;

struct rip_db *rip_db_init(void);
void	       rip_db_free(struct rip_db *);

int rip_db_add(struct rip_db *, struct rip_route_description *entry);
int rip_db_move_to_garbage(struct rip_db *, struct rip_route_description *entry);

struct rip_route_description *rip_db_get(struct rip_db *, enum rip_db_type type,
					       struct rip_route_description *entry);

int rip_db_remove(struct rip_db *, struct rip_route_description *entry);

// returns true if item was retrieved
// wrapper for hashmap iter
bool rip_db_iter(struct rip_db *, enum rip_db_type type, struct rip_db_iter *iter,
		 struct rip_route_description **desc);
bool rip_db_iter_const(struct rip_db *, enum rip_db_type type, struct rip_db_iter *iter,
		       const struct rip_route_description **desc);

size_t rip_db_count(struct rip_db *, enum rip_db_type type);

bool rip_db_any_route_changed(struct rip_db *);
void rip_db_mark_all_routes_as_unchanged(struct rip_db *db);

enum r_cmd_status rip_db_dump(FILE *file, void *data);

#endif
