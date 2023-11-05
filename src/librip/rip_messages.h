#ifndef RIP_MESSAGES_H
#define RIP_MESSAGES_H

#include <assert.h>
#include <netinet/in.h>
#include <stdalign.h>
#include <stdint.h>

#define RIP_CMD_RESPONSE 2
#define RIP_2_VERSIN 2

struct rip_header {
	uint8_t command;
	uint8_t version;
	uint8_t padding[2];
};

struct rip2_entry {
	uint16_t routing_family_id;
	uint16_t route_tag;
	struct in_addr ip_address;
	struct in_addr subnet_mask;
	struct in_addr next_hop;
	uint32_t metric;
};

void rip2_entry_ntoh(struct rip2_entry *);
void rip_header_print(const struct rip_header *);
void rip2_entry_print(const struct rip2_entry *);

static_assert(sizeof(struct rip_header) == 4, "");
static_assert(sizeof(struct rip2_entry) == 20, "");
static_assert(_Alignof(struct rip_header) == 1, "");
static_assert(_Alignof(struct rip2_entry) == 4, "");

#endif
