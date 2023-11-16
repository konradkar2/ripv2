#include "rip_common.h"
#include "logging.h"
#include <errno.h>
#include <net/if.h>
#include <netinet/in.h>
#include <string.h>

int get_prefix_len(struct in_addr subnet_mask)
{
	uint32_t host_mask     = ntohl(subnet_mask.s_addr);
	uint32_t inverted_mask = ~host_mask;
	return __builtin_clz(inverted_mask);
}

void rip_route_description_print(const struct rip_route_description *descr,
				 FILE *file)
{
	char if_name[IF_NAMESIZE] = {0};
	if (!if_indextoname(descr->next_hop_if_index, if_name)) {
		LOG_ERR("if_indextoname: %s", strerror(errno));
		strcpy(if_name, "error");
	}

	fprintf(file, "ifi %d, dev %s, ", descr->next_hop_if_index, if_name);
	fprintf(file, "rip2_entry [");
	rip2_entry_print((const struct rip2_entry *)&descr->entry, file);
	fprintf(file, "]\n");
}

