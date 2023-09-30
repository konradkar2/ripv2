#include "rip_ifc.h"
#include "logging.h"
#include <arpa/inet.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>

void rip2_entry_to_host(rip2_entry *r2e)
{
	r2e->routing_family_id = ntohs(r2e->routing_family_id);
	r2e->route_tag	       = ntohs(r2e->routing_family_id);
	// r2e->ip_address.s_addr	=
	// ntohl((uint32_t)r2e->ip_address.s_addr); r2e->subnet_mask.s_addr =
	// ntohl((uint32_t)r2e->subnet_mask.s_addr);
	// r2e->next_hop.s_addr	= ntohl((uint32_t)r2e->next_hop.s_addr);
	r2e->metric = ntohl(r2e->metric);
}

void rip_header_print(const rip_header *r_h)
{
	printf("{\n");
	printf("\tcommand: %" PRId8 "\n", r_h->command);
	printf("\tversion: %" PRId8 "\n", r_h->version);
	printf("}\n");
	fflush(stdout);
}

void rip2_entry_print(const rip2_entry *r2_e)
{
	char str[INET_ADDRSTRLEN];

	printf("{\n");
	printf("\trouting_family_id: %" PRId16 "\n", r2_e->routing_family_id);
	printf("\troute_tag: %" PRId16 "\n", r2_e->route_tag);

	inet_ntop(AF_INET, &(r2_e->ip_address.s_addr), str, INET_ADDRSTRLEN);
	printf("\tip_address: %s\n", str);

	inet_ntop(AF_INET, &(r2_e->subnet_mask.s_addr), str, INET_ADDRSTRLEN);
	printf("\tsubnet_mask: %s\n", str);

	inet_ntop(AF_INET, &(r2_e->next_hop.s_addr), str, INET_ADDRSTRLEN);
	printf("\tnext_hop: %s\n", str);
	printf("\tmetric: %" PRId8 "\n", r2_e->metric);

	printf("}\n");
	fflush(stdout);
}