#include "rip_handle_resp.h"
#include "rip_common.h"
#include "rip_db.h"
#include "rip_route.h"
#include "stdint.h"
#include "stdio.h"
#include "utils/logging.h"
#include "utils/utils.h"
#include <endian.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#define INFINITY_METRIC 16

bool is_unicast_address(struct in_addr address_n)
{
	uint32_t address  = ntohl(address_n.s_addr);
	const uint8_t msb = address >> 24;

	/*A few checks just for now */
	if (msb == 0 /*net 0*/) {
		return false;
	} else if (msb == 127 /*loopback*/) {
		return false;
	} else if (msb >= 224 /*multicast etc*/) {
		return false;
	}
	return true;
}

bool is_net_mask_valid(struct in_addr net_mask_n)
{

	uint32_t net_mask = ntohl(net_mask_n.s_addr);
	if (net_mask == 0 || net_mask == 0xFFFFFFFF) {
		return false;
	}

	size_t trailing_zeros	  = __builtin_ctz(net_mask);
	uint32_t net_mask_flipped = ~net_mask;
	size_t leading_ones	  = __builtin_clz(net_mask_flipped);

	return (leading_ones + trailing_zeros) == 32;
}

inline bool is_metric_valid(uint32_t metric) { return metric >= 1 && metric <= INFINITY_METRIC; }

static inline void update_metric(uint32_t *metric) { *metric = MIN(*metric + 1, INFINITY_METRIC); }

bool is_entry_valid(struct rip2_entry *entry)
{
	if (!is_unicast_address(entry->ip_address) || !is_net_mask_valid(entry->subnet_mask) ||
	    !is_metric_valid(entry->metric)) {
		LOG_ERR("Invalid entry:");
		rip2_entry_print(entry, stdout);
		return false;
	}

	return true;
}

int handle_non_existing_route(struct rip_route_mngr *route_mngr, struct rip_db *db,
			      struct rip_route_description *new_route)
{
	if (rip_route_add_route(route_mngr, new_route) > 0) {
		return 1;
	}

	if (rip_db_add(db, new_route)) {
		return 1;
	}

	return 0;
}

int handle_new_and_old_route(struct rip_route_mngr *route_mngr, struct rip_db *db,
			     struct rip_route_description *old_route,
			     struct rip_route_description *new_route, enum rip_state *state)
{
	(void)route_mngr;
	(void)db;
	struct rip2_entry *old_entry = &old_route->entry;
	struct rip2_entry *new_entry = &new_route->entry;

	if (old_entry->metric != new_entry->metric) {
		old_entry->metric = new_entry->metric;
		*state		  = rip_state_route_changed;
	}
	return 0;
}

void build_new_route_description(struct rip2_entry *entry, struct in_addr sender_addr, int if_index,
				 struct rip_route_description *out)
{
	memcpy(&out->entry, entry, sizeof(*entry));
	out->if_index	    = if_index;
	out->entry.next_hop = sender_addr;
	out->learned_via    = rip_route_learned_via_rip;
}

int handle_ripv2_entry(struct rip_route_mngr *route_mngr, struct rip_db *db,
		       struct rip2_entry *entry, struct in_addr sender_addr, int if_index,
		       enum rip_state *state)
{
	struct rip_route_description *old_route = NULL;
	struct rip_route_description incoming_route;
	MEMSET_ZERO(&incoming_route);

	rip2_entry_ntoh(entry);
	if (false == is_entry_valid(entry)) {
		return 0;
	}

	update_metric(&entry->metric);

	build_new_route_description(entry, sender_addr, if_index, &incoming_route);
	old_route = (struct rip_route_description *)rip_db_get(db, &incoming_route);

	if (old_route) {
		return handle_new_and_old_route(route_mngr, db, old_route, &incoming_route, state);
	} else {
		return handle_non_existing_route(route_mngr, db, &incoming_route);
	}
}

int rip_handle_response(struct rip_route_mngr *route_mngr, struct rip_db *db,
			struct rip2_entry entries[], size_t n_entry, struct in_addr sender_addr,
			int origin_if_index, enum rip_state *state)
{
	int ret = 0;
	for (size_t i = 0; i < n_entry; ++i) {
		struct rip2_entry *entry = &entries[i];
		if (handle_ripv2_entry(route_mngr, db, entry, sender_addr, origin_if_index,
				       state)) {
			ret = 1;
		}
	}

	return ret;
}
