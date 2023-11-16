#include "rip_db.h"
#include "logging.h"
#include "rip_common.h"
#include "rip_ipc.h"
#include "rip_messages.h"
#include "utils.h"
#include "utils/hashmap.h"
#include <errno.h>
#include <net/if.h>
#include <stdio.h>
#include <string.h>
#include <utils/vector.h>

static bool rip2_entry_cmp(const void *el_a, const void *el_b)
{
	const struct rip2_entry *a = el_a;
	const struct rip2_entry *b = el_b;

	return a->ip_address.s_addr == b->ip_address.s_addr &&
	       a->next_hop.s_addr == b->next_hop.s_addr &&
	       a->subnet_mask.s_addr == b->subnet_mask.s_addr;
}

static int rip_route_description_cmp(const void *el_a, const void *el_b,
				     void *udata)
{
	(void)udata;
	const struct rip_route_description *a = el_a;
	const struct rip_route_description *b = el_b;

	return a->next_hop_if_index == b->next_hop_if_index &&
	       rip2_entry_cmp(&a->entry, &b->entry);
}

uint64_t rip_route_description_cmp_hash(const void *item, uint64_t seed0,
					uint64_t seed1)
{
	const struct rip_route_description *route_descr = item;
	return hashmap_sip(route_descr, sizeof(struct rip_route_description),
			   seed0, seed1);
}

int rip_db_init(struct rip_db *db)
{
	db->added_routes = hashmap_new(sizeof(struct rip_route_description), 0,
				       0, 0, rip_route_description_cmp_hash,
				       rip_route_description_cmp, NULL, NULL);
	if (!db->added_routes) {
		LOG_ERR("vector_create");
		return 1;
	}
	return 0;
}
void rip_db_destroy(struct rip_db *db)
{
	if (db && db->added_routes)
		hashmap_free(db->added_routes);
}

int rip_db_add(struct rip_db *db, struct rip_route_description *entry)
{
	if (rip_db_contains(db, entry)) {
		LOG_ERR("rip_db_contains: element already added");
		return 1;
	}

	if (hashmap_set(db->added_routes, entry)) {
		LOG_ERR("rip_db_add: hashmap_set");
		return 1;
	}
	return 0;
}

bool rip_db_contains(struct rip_db *db, struct rip_route_description *entry)
{
	if (hashmap_get(db->added_routes, entry)) {
		return false;
	}
	return true;
}

int rip_db_remove(struct rip_db *db, struct rip_route_description *entry)
{
	if (hashmap_delete(db->added_routes, entry)) {
		LOG_ERR("rip_db_remove: element not found");
		return 1;
	}

	return 0;
}

bool rip_route_description_iter(const void *item, void *udata)
{
	const struct rip_route_description *descr = item;
	FILE *file				  = udata;

	rip_route_description_print(descr, file);
	return true;
}

enum r_cmd_status rip_db_dump(FILE *file, void *data)
{
	const struct rip_db *db = data;
	hashmap_scan(db->added_routes, rip_route_description_iter, file);

	return r_cmd_status_success;
}
