#ifndef RIP_COMMON_H
#define RIP_COMMON_H

#include <assert.h>
#include <net/if.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define RIP_CMD_REQUEST 1
#define RIP_CMD_RESPONSE 2
#define RIP_2_VERSIN 2
#define RIP_MULTICAST_ADDR "224.0.0.9"
#define RIP_UDP_PORT 520

struct rip_header {
	uint8_t command;
	uint8_t version;
	uint8_t padding[2];
};

struct rip2_entry {
	uint16_t       routing_family_id;
	uint16_t       route_tag;
	struct in_addr ip_address;
	struct in_addr subnet_mask;
	struct in_addr next_hop;
	uint32_t       metric;
};

struct msg_buffer {
	struct rip_header header;
	struct rip2_entry entries[500];
};

/* converts all the fields to network, except for addresses*/
void rip2_entry_ntoh(struct rip2_entry *);
void rip2_entry_hton(struct rip2_entry *);

void rip_header_print(struct rip_header);
void rip2_entry_print(struct rip2_entry, FILE *file);

static_assert(sizeof(struct rip_header) == 4, "");
static_assert(sizeof(struct rip2_entry) == 20, "");

// Aggregate type for lookup
// Check rip_db.c for which field is used for lookup and cannot be modified
struct rip_route_description {
	struct rip2_entry entry;
	uint32_t	  if_index;

	/* fields not used for hashmap comparison */
	// changed, but not yet advertised
	bool changed;
	// when reaches 180, begin deletion process
	int timeout_cnt;
};

struct rip_socket {
	int  fd;
	char if_name[IF_NAMESIZE];
	int  if_index;
};

struct rip_ifc {
	struct rip_socket socket_rx;
	struct rip_socket socket_tx;
};

struct rip_ifc_vec {
	struct rip_ifc *items;
	size_t		count;
	size_t		capacity;
};

int  get_prefix_len(struct in_addr subnet_mask);
int  prefix_len_to_subnet(size_t prefix_len, struct in_addr *out);
void rip_route_description_print(const struct rip_route_description *descr, FILE *file);

bool is_unicast_address(struct in_addr address_n);
bool is_net_mask_valid(struct in_addr net_mask_n);

#endif
