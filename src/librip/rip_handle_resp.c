#include "rip_handle_resp.h"
#include "logging.h"
#include "rip_common.h"
#include "rip_db.h"
#include "rip_if.h"
#include "rip_route.h"
#include "stdint.h"
#include "stdio.h"
#include "utils.h"
#include <endian.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdlib.h>
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

inline bool is_metric_valid(uint32_t metric)
{
	return metric >= 1 && metric <= INFINITY_METRIC;
}

static inline void update_metric(uint32_t *metric)
{
	*metric = MIN(*metric + 1, INFINITY_METRIC);
}

int handle_response(struct rip_route_mngr *route_mngr, struct rip_db *db,
		    struct rip2_entry entries[], size_t n_entry,
		    struct in_addr sender_addr, int origin_if_index)
{
	for (size_t i = 0; i < n_entry; ++i) {
		struct rip2_entry *entry = &entries[i];

		// printf("[%zu]\n", i);
		rip2_entry_ntoh(entry);
		// rip2_entry_print(entry);

		if (!is_unicast_address(entry->ip_address) ||
		    !is_net_mask_valid(entry->subnet_mask) ||
		    !is_metric_valid(entry->metric)) {
			LOG_ERR("Invalid entry:");
			rip2_entry_print(entry);
			return 0;
		}

		LOG_INFO("Valid entry!");
		update_metric(&entry->metric);
		if (entry->metric == INFINITY_METRIC) {
			LOG_ERR("Max metric reached");
			return 0;
		}

		entry->next_hop				 = sender_addr;
		struct rip_route_description route_descr = {
		    .entry	       = *entry, // TODO: optimize
		    .next_hop_if_index = origin_if_index};
		if (!rip_db_add(db, &route_descr)) {

			LOG_ERR("Entry already exists");
			return 0;
		}

		rip_route_entry *route_entry =
		    rip_route_entry_create(&route_descr);
		if (!route_entry) {
			LOG_ERR("rip_route_entry_create");
			return 0;
		}

		if (rip_route_add_route(route_mngr, route_entry) > 0) {
			LOG_ERR("rip_route_add_route");
		}
		rip_route_entry_free(route_entry);
		return 0;
	}

	return 0;
}
