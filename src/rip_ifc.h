#ifndef RIP_IFC
#define RIP_IFC

#include <netinet/in.h>
#include <stdint.h>
#include <assert.h>

typedef struct rip_header {
	uint8_t command;
	uint8_t version;
	uint8_t padding[2];
} rip_header;

typedef struct rip2_entry {
	uint16_t routing_family_id;
	uint16_t route_tag;
	struct in_addr ip_address;
	struct in_addr subnet_mask;
	struct in_addr next_hop;
	uint32_t metric;
} rip2_entry;
void rip2_entry_to_host(rip2_entry*);


void rip_header_print(const rip_header*);
void rip2_entry_print(const rip2_entry*);

static_assert(sizeof(rip_header) == 4);
static_assert(sizeof(rip2_entry) == 20);

#endif