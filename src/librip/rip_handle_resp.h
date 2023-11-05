#ifndef RIP_HANDLE_RESPONE_H
#define RIP_HANDLE_RESPONE_H

#include "rip.h"
#include "rip_messages.h"
#include <netinet/in.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

int handle_response(struct rip_route_mngr * route_mngr, struct rip2_entry entries[],
		    size_t n_entry, struct in_addr sender_addr, int origin_if_index);


bool is_unicast_address(uint32_t address);
bool is_net_mask_valid(uint32_t net_mask);

bool is_metric_valid(uint32_t metric);

#endif
