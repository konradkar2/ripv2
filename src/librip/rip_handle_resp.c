#include "rip_handle_resp.h"
#include "stdint.h"
#include "stdio.h"

int handle_response(rip_context *ctx, rip2_entry *entries, size_t n_entry,
		    struct in_addr sender)
{
	(void)ctx;
	(void)sender;

	for (size_t i = 0; i < n_entry; ++i) {
		rip2_entry *entry = &entries[i];

		printf("[%zu]\n", i);
		rip2_entry_to_host(entry);
		rip2_entry_print(entry);

		uint8_t *sender_bytes = (uint8_t *)&sender.s_addr;
		printf("sender: %d.%d.%d.%d\n", sender_bytes[0], sender_bytes[1],
		       sender_bytes[2], sender_bytes[3]);

		uint8_t *entry_ip_bytes = (uint8_t *)&entry->ip_address.s_addr;
		printf("entry: %d.%d.%d.%d\n", entry_ip_bytes[0], entry_ip_bytes[1],
		       entry_ip_bytes[2], entry_ip_bytes[3]);
	}

	return 0;
}