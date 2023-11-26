#include "rip_common.h"
#include "utils/logging.h"
#include <arpa/inet.h>
#include <errno.h>
#include <inttypes.h>
#include <net/if.h>
#include <netinet/in.h>
#include <stdint.h>
#include <string.h>

int get_prefix_len(struct in_addr subnet_mask)
{
	uint32_t host_mask     = ntohl(subnet_mask.s_addr);
	uint32_t inverted_mask = ~host_mask;
	return __builtin_clz(inverted_mask);
}

void rip_route_description_print(const struct rip_route_description *descr, FILE *file)
{
	char if_name[IF_NAMESIZE] = {0};
	if (!if_indextoname(descr->if_index, if_name)) {
		LOG_ERR("if_indextoname: %s", strerror(errno));
		strcpy(if_name, "error");
	}

	fprintf(file, "ifi %d, dev %s, ", descr->if_index, if_name);
	rip2_entry_print((const struct rip2_entry *)&descr->entry, file);
	fprintf(file, "\n");
}

void rip2_entry_ntoh(struct rip2_entry *r2e)
{
	r2e->routing_family_id = ntohs(r2e->routing_family_id);
	r2e->route_tag	       = ntohs(r2e->routing_family_id);
	r2e->metric	       = ntohl(r2e->metric);
}

void rip2_entry_hton(struct rip2_entry *r2e)
{
	r2e->routing_family_id = htons(r2e->routing_family_id);
	r2e->route_tag	       = htons(r2e->routing_family_id);
	r2e->metric	       = htonl(r2e->metric);
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
