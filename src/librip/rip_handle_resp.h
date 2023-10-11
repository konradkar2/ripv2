#ifndef RIP_HANDLE_RESPONE_H
#define RIP_HANDLE_RESPONE_H

#include "rip.h"
#include "rip_messages.h"
#include <netinet/in.h>
#include <stdbool.h>
#include <stddef.h>
int handle_response(rip_context *ctx, rip2_entry entries[], size_t n_entry,
		    const struct in_addr sender);

bool is_unicast_address(struct in_addr address);
bool is_net_mask_valid(struct in_addr net_mask);

#endif