#include "rip_messages.h"
#include "logging.h"
#include "rip_common.h"
#include <arpa/inet.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>

void rip2_entry_ntoh(struct rip2_entry *r2e)
{
	r2e->routing_family_id = ntohs(r2e->routing_family_id);
	r2e->route_tag	       = ntohs(r2e->routing_family_id);
	// r2e->ip_address	       = ntohl(r2e->ip_address);
	// r2e->subnet_mask       = ntohl(r2e->subnet_mask);
	// r2e->next_hop	       = ntohl(r2e->next_hop);
	r2e->metric = ntohl(r2e->metric);
}

void rip_header_print(const struct rip_header *r_h)
{
	printf("{\n");
	printf("\tcommand: %" PRId8 "\n", r_h->command);
	printf("\tversion: %" PRId8 "\n", r_h->version);
	printf("}\n");
	fflush(stdout);
}

void rip2_entry_print(const struct rip2_entry *r2_e, FILE *file)
{
	char str[INET_ADDRSTRLEN];

	fprintf(file, "rfamily_id %" PRId16 ", ", r2_e->routing_family_id);
	fprintf(file, "rtag %" PRId16 ", ", r2_e->route_tag);

	inet_ntop(AF_INET, &(r2_e->ip_address), str, INET_ADDRSTRLEN);
	int prefix_len = get_prefix_len(r2_e->subnet_mask);
	fprintf(file, "network %s/%d, ", str, prefix_len);

	inet_ntop(AF_INET, &(r2_e->next_hop), str, INET_ADDRSTRLEN);
	fprintf(file, "nh %s, ", str);
	fprintf(file, "metric %" PRId8 "", r2_e->metric);
}
