#include "rip_handle_resp.h"
#include "logging.h"
#include "stdint.h"
#include "stdio.h"
#include <endian.h>
#include <netinet/in.h>
#include <stdint.h>
#include <sys/types.h>

#ifndef __BYTE_ORDER__
#error no endianess defined
#endif

bool is_unicast_address(struct in_addr address)
{
	const uint8_t *octets = (const uint8_t *)&address;

	/*A few checks just for now */
	if (octets[0] == 0 /*net 0*/) {
		return false;
	} else if (octets[0] == 127 /*loopback*/) {
		return false;
	} else if (octets[0] >= 224 /*multicast etc*/) {
		return false;
	}
	return true;
}

bool is_net_mask_valid(struct in_addr net_mask)
{
	const uint32_t net_mask_host = ntohl(net_mask.s_addr);
	uint32_t mask		     = 0xffffffff;

	for (size_t i = 0; i < 32; ++i) {		
		if ((mask & net_mask_host) == mask) {
			return true;
		}
		mask <<= 1;
	}
	return false;
}

int handle_response(rip_context *ctx, rip2_entry entries[], size_t n_entry,
		    const struct in_addr sender)
{
	(void)ctx;
	(void)sender;

	for (size_t i = 0; i < n_entry; ++i) {
		rip2_entry *entry = &entries[i];

		printf("[%zu]\n", i);
		rip2_entry_ntoh(entry);
		rip2_entry_print(entry);

		if (is_unicast_address(entry->ip_address) ||
		    is_net_mask_valid(entry->subnet_mask)) {
			LOG_INFO("Valid!");
		} else {
			LOG_ERR("Invalid entry:");
			rip2_entry_print(entry);
		}
	}

	return 0;
}