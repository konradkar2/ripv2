#ifndef RIP_UPDATE_H
#define RIP_UPDATE_H

#include <netinet/in.h>
#include <rip_common.h>
#include <rip_db.h>

struct rip_context;
int rip_send_advertisement_multicast(struct rip_context *ctx, bool advertise_only_changed);
int rip_send_request_multicast(struct rip_context *ctx);
int rip_send_advertisement_unicast(struct rip_db *db, struct rip2_entry entries[], size_t n_entry,
				   struct in_addr sender_addr, int origin_if_index);
void rip_send_advertisement_shutdown(struct rip_context *ctx);

#endif
