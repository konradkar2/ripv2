#ifndef RIP_COMMON_H
#define RIP_COMMON_H

#include "rip_messages.h"
#include <assert.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>

struct rip_route_description {
	struct rip2_entry entry;
	uint32_t if_index;
};

struct rip_event;
typedef int (*rip_event_cb)(const struct rip_event *);

struct rip_event {
	int fd;
	rip_event_cb cb;
	void *arg1;
};

int get_prefix_len(struct in_addr subnet_mask);
void rip_route_description_print(const struct rip_route_description *descr, FILE *file);

#endif
