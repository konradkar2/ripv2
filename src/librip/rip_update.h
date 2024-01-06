#ifndef RIP_UPDATE_H
#define RIP_UPDATE_H

#include <netinet/in.h>
#include <rip_common.h>
#include <rip_db.h>

struct rip_context;
int rip_send_advertisement(struct rip_context *ctx);

// response to request message
int rip_send_advertisement_unicast(struct rip_db *db, struct rip2_entry entries[], size_t n_entry,
				   struct in_addr sender_addr, int origin_if_index);

#endif
