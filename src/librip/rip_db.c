#include "rip_db.h"
#include "rip_common.h"
#include "rip_ipc.h"
#include "utils/hashmap.h"
#include "utils/logging.h"
#include <net/if.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <utils/vector.h>

#define VALUE_FIELD_CMP(obj_a, obj_b, field)                                                       \
	if (obj_a->field != obj_b->field) {                                                        \
		return (obj_a->field > obj_b->field) - (obj_a->field < obj_b->field);              \
	}

static int rip_route_description_cmp(const void *el_a, const void *el_b, void *udata)
{
	(void)udata;
	const struct rip_route_description *a	    = el_a;
	const struct rip_route_description *b	    = el_b;
	const struct rip2_entry		   *entry_a = &a->entry;
	const struct rip2_entry		   *entry_b = &b->entry;

	VALUE_FIELD_CMP(a, b, if_index)
	VALUE_FIELD_CMP(entry_a, entry_b, ip_address.s_addr)
	VALUE_FIELD_CMP(entry_a, entry_b, subnet_mask.s_addr)
	VALUE_FIELD_CMP(entry_a, entry_b, next_hop.s_addr)

	return 0;
}

static uint64_t rip_route_description_cmp_hash(const void *item, uint64_t seed0, uint64_t seed1)
{
	const struct rip_route_description *route_descr = item;

	struct rip_route_key {
		struct in_addr ip_address;
		struct in_addr subnet_mask;
		struct in_addr next_hop;
		uint32_t       if_index;
	} key = {
	    .ip_address	 = route_descr->entry.ip_address,
	    .subnet_mask = route_descr->entry.subnet_mask,
	    .next_hop	 = route_descr->entry.next_hop,
	    .if_index	 = route_descr->if_index,
	};

	return hashmap_sip(&key, sizeof(key), seed0, seed1);
}

int rip_db_init(struct rip_db *db)
{
	db->any_route_changed = false;
	db->added_routes =
	    hashmap_new(sizeof(struct rip_route_description), 0, 0, 0,
			rip_route_description_cmp_hash, rip_route_description_cmp, NULL, NULL);
	if (!db->added_routes) {
		LOG_ERR("hashmap_new");
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
	const struct rip_route_description *old;

	entry->changed = true;
	if ((old = hashmap_set(db->added_routes, entry))) {
		LOG_ERR("element already added, old: ");
		rip_route_description_print(old, stdout);
		LOG_INFO("new: ");
		rip_route_description_print(entry, stdout);
		return 1;
	}

	if (hashmap_oom(db->added_routes)) {
		LOG_ERR("hashmap is out of memmory");
		return 1;
	}

	db->any_route_changed = true;
	return 0;
}

const struct rip_route_description *rip_db_get(struct rip_db		    *db,
					       struct rip_route_description *entry)
{
	return hashmap_get(db->added_routes, entry);
}

int rip_db_remove(struct rip_db *db, struct rip_route_description *entry)
{
	if (hashmap_delete(db->added_routes, entry)) {
		LOG_ERR("rip_db_remove: element not found");
		return 1;
	}

	return 0;
}

enum r_cmd_status rip_db_dump(FILE *file, void *data)
{
	const struct rip_db *db = data;

	size_t i = 0;
	void  *el;

	while (hashmap_iter(db->added_routes, &i, &el)) {
		const struct rip_route_description *descr = el;
		rip_route_description_print(descr, file);
	}

	return r_cmd_status_success;
}

bool rip_db_iter(struct rip_db *db, size_t *iter, const struct rip_route_description **desc)
{
	void *el;
	bool  ret = hashmap_iter(db->added_routes, iter, &el);
	if (ret) {
		*desc = el;
		return true;
	}

	return false;
}

bool rip_db_any_route_changed(struct rip_db *db) { return db->any_route_changed; }

void rip_db_mark_all_routes_as_unchanged(struct rip_db *db)
{
	if (db->any_route_changed) {
		LOG_INFO("%s", __func__);
		
		size_t			      iter  = 0;
		struct rip_route_description *entry = NULL;

		while (hashmap_iter(db->added_routes, &iter, (void **)&entry)) {
			entry->changed = false;
		}
		db->any_route_changed = false;
	}
}
