#ifndef RIP_HANDLE_RESPONE_H
#define RIP_HANDLE_RESPONE_H

#include "rip.h"
#include "rip_common.h"
#include <netinet/in.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

int rip_handle_response(struct rip_route_mngr *route_mngr, struct rip_db *db,
		    struct rip2_entry entries[], size_t n_entry,
		    struct in_addr sender_addr, int if_index, enum rip_state * state);

bool is_unicast_address(struct in_addr address_n);
bool is_net_mask_valid(struct in_addr net_mask_n);

bool is_metric_valid(uint32_t metric);

#endif
