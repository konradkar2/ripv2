#include "rip_db.h"
#include "rip_common.h"
#include "rip_ipc.h"
#include "utils/hashmap.h"
#include "utils/logging.h"
#include "utils/utils.h"
#include <net/if.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>

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

struct rip_db {
	struct hashmap *ok_routes;
	struct hashmap *garbage_routes;
	bool		any_route_changed;
};

struct rip_db *rip_db_init(void)
{
	struct rip_db *db = CALLOC(sizeof(struct rip_db));
	if (!db) {
		LOG_ERR("calloc");
		return NULL;
	}

	db->any_route_changed = false;
	db->ok_routes =
	    hashmap_new(sizeof(struct rip_route_description), 0, 0, 0,
			rip_route_description_cmp_hash, rip_route_description_cmp, NULL, NULL);

	db->garbage_routes =
	    hashmap_new(sizeof(struct rip_route_description), 0, 0, 0,
			rip_route_description_cmp_hash, rip_route_description_cmp, NULL, NULL);

	if (!db->ok_routes || !db->garbage_routes) {
		LOG_ERR("hashmap_new");
		rip_db_free(db);
		return NULL;
	}

	return db;
}
void rip_db_free(struct rip_db *db)
{
	if (!db) {
		return;
	}

	if (db->ok_routes)
		hashmap_free(db->ok_routes);

	if (db->garbage_routes)
		hashmap_free(db->garbage_routes);

	free(db);
}

int rip_db_add(struct rip_db *db, struct rip_route_description *entry)
{
	const struct rip_route_description *old;

	entry->changed = true;
	if ((old = hashmap_set(db->ok_routes, entry))) {
		LOG_ERR("element already added, old: ");
		rip_route_description_print(old, stdout);
		LOG_INFO("new: ");
		rip_route_description_print(entry, stdout);
		return 1;
	}

	if (hashmap_oom(db->ok_routes)) {
		LOG_ERR("hashmap is out of memmory");
		return 1;
	}

	db->any_route_changed = true;

	LOG_INFO("%s: ", __func__);
	rip_route_description_print(entry, stdout);
	return 0;
}

struct rip_route_description *rip_db_get(struct rip_db *db, enum rip_db_type type,
					 struct rip_route_description *entry)
{
	//return (void *)hashmap_get(db->ok_routes, entry);

	switch (type) {
	case rip_db_ok:
		return (void *)hashmap_get(db->ok_routes, entry);
	case rip_db_garbage:
		return (void *)hashmap_get(db->garbage_routes, entry);
	case rip_db_all: {
		void *el = (void *)hashmap_get(db->ok_routes, entry);
		if (el) {
			return el;
		} else {
			return (void *)hashmap_get(db->garbage_routes, entry);
		}
	}
	default:
		UNREACHABLE();
	}
}

int rip_db_remove(struct rip_db *db, struct rip_route_description *entry)
{
	const void *el = NULL;
	el	       = hashmap_delete(db->ok_routes, entry);
	if (!el) {
		el = hashmap_delete(db->garbage_routes, entry);
	}

	if (!el) {
		LOG_ERR("element not found");
		return 1;
	}

	return 0;
}

int rip_db_move_to_garbage(struct rip_db *db, struct rip_route_description *entry)
{
	if (entry->changed)
		db->any_route_changed = true;

	const void *el = NULL;
	el	       = hashmap_delete(db->ok_routes, entry);
	if (!el) {
		LOG_ERR("item not found");
		return 1;
	}

	if (hashmap_set(db->garbage_routes, el)) {
		LOG_ERR("item already in garbage routes");
		return 1;
	}

	return 0;
}

enum r_cmd_status rip_db_dump(FILE *file, void *data)
{
	struct rip_db *db = data;

	struct rip_db_iter		    iter  = {0};
	const struct rip_route_description *descr = NULL;
	while (rip_db_iter_const(db, rip_db_all, &iter, &descr)) {
		rip_route_description_print(descr, file);
	}

	return r_cmd_status_success;
}

static bool rip_db_iter_garbage(struct rip_db *db, struct rip_db_iter *iter,
				struct rip_route_description **desc)
{
	void *el;
	bool  ret = hashmap_iter(db->garbage_routes, &iter->garbage_routes_iter, &el);
	if (ret) {
		*desc = el;
		return true;
	}

	return false;
}

static bool rip_db_iter_ok(struct rip_db *db, struct rip_db_iter *iter,
			   struct rip_route_description **desc)
{
	void *el;
	bool  ret = hashmap_iter(db->ok_routes, &iter->ok_routes_iter, &el);
	if (ret) {
		*desc = el;
		return true;
	}

	return false;
}

static bool rip_db_iter_all(struct rip_db *db, struct rip_db_iter *iter,
			    struct rip_route_description **desc)
{

	if (iter->ok_routes_iterated) {
		return rip_db_iter_garbage(db, iter, desc);
	}

	bool ret = rip_db_iter_ok(db, iter, desc);
	if (ret) {
		return true;
	} else {
		iter->ok_routes_iterated = true;
		return rip_db_iter_garbage(db, iter, desc);
	}
}

bool rip_db_iter(struct rip_db *db, enum rip_db_type type, struct rip_db_iter *iter,
		 struct rip_route_description **desc)
{
	switch (type) {
	case rip_db_ok:
		return rip_db_iter_ok(db, iter, desc);
	case rip_db_garbage:
		return rip_db_iter_garbage(db, iter, desc);
	case rip_db_all:
		return rip_db_iter_all(db, iter, desc);
	default:
		UNREACHABLE();
	}
}

bool rip_db_iter_const(struct rip_db *db, enum rip_db_type type, struct rip_db_iter *iter,
		       const struct rip_route_description **desc)
{
	return rip_db_iter(db, type, iter, (struct rip_route_description **)desc);
}

size_t rip_db_count(struct rip_db *db, enum rip_db_type type)
{
	switch (type) {
	case rip_db_ok:
		return hashmap_count(db->ok_routes);
	case rip_db_garbage:
		return hashmap_count(db->garbage_routes);
	case rip_db_all:
		return hashmap_count(db->ok_routes) + hashmap_count(db->garbage_routes);
	default:
		UNREACHABLE();
	}
}

bool rip_db_any_route_changed(struct rip_db *db) { return db->any_route_changed; }

void rip_db_mark_all_routes_as_unchanged(struct rip_db *db)
{
	if (db->any_route_changed) {
		LOG_INFO("%s", __func__);

		struct rip_db_iter	      iter  = {0};
		struct rip_route_description *entry = NULL;

		while (rip_db_iter(db, rip_db_all, &iter, &entry)) {
			entry->changed = false;
		}
		db->any_route_changed = false;
	}
}
