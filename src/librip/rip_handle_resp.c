#include "rip_handle_resp.h"
#include "logging.h"
#include "rip_common.h"
#include "rip_db.h"
#include "rip_route.h"
#include "stdint.h"
#include "stdio.h"
#include "utils.h"
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

int handle_entry(struct rip_route_mngr *route_mngr, struct rip_db *db, struct rip2_entry *entry,
		 struct in_addr sender_addr, int origin_if_index)
{
	const struct rip_route_description *old_route = NULL;
	struct rip_route_description incoming_route   = {0};

	rip2_entry_ntoh(entry);
	if (false == is_entry_valid(entry)) {
		return 0;
	}

	update_metric(&entry->metric);
	if (entry->metric >= INFINITY_METRIC) {
		rip2_entry_print(entry, stdout);
		LOG_ERR("Max metric reached");
		return 0;
	}

	memcpy(&incoming_route.entry, entry, sizeof(*entry));
	incoming_route.if_index	      = origin_if_index;
	incoming_route.entry.next_hop = sender_addr;
	rip_route_description_print(&incoming_route, stdout);

	old_route = rip_db_get(db, &incoming_route);
	if (old_route) {
		/*network already exists, check if e.g. prefix is longer, and
		 * replace it */
		// for now do nothing
		return 0;
	}

	if (rip_route_add_route(route_mngr, &incoming_route) > 0) {
		return 1;
	}

	if (rip_db_add(db, &incoming_route)) {
		return 1;
	}

	return 0;
}

int handle_response(struct rip_route_mngr *route_mngr, struct rip_db *db, struct rip2_entry entries[], size_t n_entry,
		    struct in_addr sender_addr, int origin_if_index)
{
	int ret = 0;
	for (size_t i = 0; i < n_entry; ++i) {
		struct rip2_entry *entry = &entries[i];
		if (handle_entry(route_mngr, db, entry, sender_addr, origin_if_index)) {
			ret = 1;
		}
	}

	return ret;
}
