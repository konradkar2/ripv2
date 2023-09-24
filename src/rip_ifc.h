#ifndef RIP_IFC
#define RIP_IFC

#include <stdint.h>
#include <assert.h>

#define RIP_PORT 520

typedef struct rip_header {
	uint8_t command;
	uint8_t version;
	uint8_t padding[2];
} rip_header;

typedef struct rip2_entry {
	uint16_t routing_family_id;
	uint16_t route_tag;
	uint32_t ip_address;
	uint32_t subnet_mask;
	uint32_t next_hop;
	uint32_t metric;
} rip2_entry;


static_assert(sizeof(rip_header) == 4);
static_assert(sizeof(rip2_entry) == 20);

#endif