#include "rip_db.h"
#include "logging.h"
#include "rip_common.h"
#include "rip_ipc.h"
#include "rip_messages.h"
#include "utils.h"
#include "utils/hashmap.h"
#include <errno.h>
#include <net/if.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <utils/vector.h>

/* static bool rip2_entry_cmp(const void *el_a, const void *el_b)
{
	const struct rip2_entry *a = el_a;
	const struct rip2_entry *b = el_b;

	return a->ip_address.s_addr == b->ip_address.s_addr &&
	       a->next_hop.s_addr == b->next_hop.s_addr &&
	       a->subnet_mask.s_addr == b->subnet_mask.s_addr;
} */

static int rip_route_description_cmp(const void *el_a, const void *el_b,
				     void *udata)
{
	(void)udata;
	const struct rip_route_description *a = el_a;
	const struct rip_route_description *b = el_b;

	return a->entry.ip_address.s_addr == b->entry.ip_address.s_addr &&
	       a->entry.subnet_mask.s_addr == b->entry.subnet_mask.s_addr &&
	       a->entry.next_hop.s_addr == b->entry.next_hop.s_addr &&
	       a->next_hop_if_index == b->next_hop_if_index;
}

static uint64_t rip_route_description_cmp_hash(const void *item, uint64_t seed0,
					       uint64_t seed1)
{
	const struct rip_route_description *route_descr = item;

	struct rip_route_key {
		struct in_addr ip_address;
		struct in_addr subnet_mask;
		struct in_addr next_hop;
		uint32_t if_index;
	} key = {
	    .ip_address	 = route_descr->entry.ip_address,
	    .subnet_mask = route_descr->entry.subnet_mask,
	    .next_hop	 = route_descr->entry.next_hop,
	    .if_index	 = route_descr->next_hop_if_index,
	};

	return hashmap_sip(&key, sizeof(key), seed0,
			   seed1); // take ip, subnet, nexthop, and nextop index
}

int rip_db_init(struct rip_db *db)
{
	db->added_routes = hashmap_new(sizeof(struct rip_route_description), 0,
				       0, 0, rip_route_description_cmp_hash,
				       rip_route_description_cmp, NULL, NULL);
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
	if (NULL != rip_db_get(db, entry)) {
		LOG_ERR("rip_db_contains: element already added");
		return 1;
	}

	if (hashmap_set(db->added_routes, entry)) {
		LOG_ERR("rip_db_add: hashmap_set");
		return 1;
	}
	return 0;
}

const struct rip_route_description *
rip_db_get(struct rip_db *db, struct rip_route_description *entry)
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

static bool rip_route_description_iter_print(const void *item, void *udata)
{
	const struct rip_route_description *descr = item;
	FILE *file				  = udata;

	rip_route_description_print(descr, file);
	return true;
}

enum r_cmd_status rip_db_dump(FILE *file, void *data)
{
	const struct rip_db *db = data;
	hashmap_scan(db->added_routes, rip_route_description_iter_print, file);

	return r_cmd_status_success;
}
