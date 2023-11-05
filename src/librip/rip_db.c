#include "rip_db.h"
#include "logging.h"
#include "rip_common.h"
#include "rip_messages.h"
#include <utils/vector.h>
#include <string.h>

int rip_db_init(struct rip_db *db)
{
	db->added_routes =
	    vector_create(128, sizeof(struct rip_route_description));
	if (!db->added_routes) {
		LOG_ERR("vector_create");
		return 1;
	}
	return 0;
}
void rip_db_destroy(struct rip_db *db)
{
	if (db->added_routes)
		vector_free(db->added_routes);
}

bool rip_db_add(struct rip_db *db, struct rip_route_description *entry)
{
	if (rip_db_contains(db, entry)) {
		LOG_ERR("rip_db_contains: element already added");
		return false;
	}
	if (vector_add(db->added_routes, (void *)entry,
		       sizeof(struct rip_route_description))) {
		LOG_ERR("rip_db_add: vector_add_el");
		return false;
	}
	return true;
}

static bool rip2_entry_cmp(void *el_a, void *el_b)
{
	struct rip2_entry *a = el_a;
	struct rip2_entry *b = el_b;

	return a->ip_address.s_addr == b->ip_address.s_addr &&
	       a->next_hop.s_addr == b->next_hop.s_addr &&
	       a->subnet_mask.s_addr == b->subnet_mask.s_addr;
}

static bool rip_route_description_cmp(void *el_a, void *el_b)
{
	struct rip_route_description *a = el_a;
	struct rip_route_description *b = el_b;

	return a->next_hop_if_index == b->next_hop_if_index &&
	       rip2_entry_cmp(&a->entry, &b->entry);
}

bool rip_db_contains(struct rip_db *db, struct rip_route_description *entry)
{

	ssize_t idx =
	    vector_find(db->added_routes, entry, rip_route_description_cmp);
	if (idx == -1) {
		return false;
	}
	return true;
}

bool rip_db_remove(struct rip_db *db, struct rip_route_description *entry)
{
	ssize_t idx =
	    vector_find(db->added_routes, entry, rip_route_description_cmp);
	if (idx == -1) {
		return false;
	}
	vector_del(db->added_routes, idx);

	return 0;
}
