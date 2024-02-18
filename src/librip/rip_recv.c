#include "rip_recv.h"
#include "rip.h"
#include "rip_common.h"
#include "rip_db.h"
#include "rip_route.h"
#include "rip_update.h"
#include "stdint.h"
#include "stdio.h"
#include "utils/logging.h"
#include "utils/utils.h"
#include <endian.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#define INFINITY_METRIC 16

inline bool is_metric_valid(uint32_t metric) { return metric >= 1 && metric <= INFINITY_METRIC; }

static inline void update_metric(uint32_t *metric) { *metric = MIN(*metric + 1, INFINITY_METRIC); }

bool is_entry_valid(struct rip2_entry *entry)
{
	if (!is_unicast_address(entry->ip_address) || !is_net_mask_valid(entry->subnet_mask) ||
	    !is_metric_valid(entry->metric)) {
		LOG_ERR("Invalid entry:");
		rip2_entry_print(*entry, stdout);
		return false;
	}

	return true;
}

int handle_non_existing_route(struct rip_route_mngr *route_mngr, struct rip_db *db,
			      struct rip_route_description *new_route)
{
	new_route->changed = true;
	if (rip_route_add_route(route_mngr, new_route)) {
		return 1;
	}

	if (rip_db_add(db, new_route)) {
		return 1;
	}

	return 0;
}

int handle_route_update(struct rip_route_mngr *route_mngr, struct rip_db *db,
			struct rip_route_description *old_route,
			struct rip_route_description *new_route)
{
	(void)route_mngr;
	(void)db;
	struct rip2_entry *old_entry = &old_route->entry;
	struct rip2_entry *new_entry = &new_route->entry;

	if (old_entry->metric != new_entry->metric) {
		old_entry->metric = new_entry->metric;
	}
	return 0;
}

void build_route_description(struct rip2_entry *entry, struct in_addr sender_addr, int if_index,
			     struct rip_route_description *out)
{
	memcpy(&out->entry, entry, sizeof(*entry));
	out->if_index	    = if_index;
	out->entry.next_hop = sender_addr;
	out->learned_via    = rip_route_learned_via_rip;
}

int handle_ripv2_entry(struct rip_route_mngr *route_mngr, struct rip_db *db,
		       struct rip2_entry *entry, struct in_addr sender_addr, int if_index)
{
	struct rip_route_description *old_route = NULL;
	struct rip_route_description  incoming_route;
	MEMSET_ZERO(&incoming_route);

	rip2_entry_ntoh(entry);
	if (false == is_entry_valid(entry)) {
		return 0;
	}
	update_metric(&entry->metric);
	rip2_entry_hton(entry);

	build_route_description(entry, sender_addr, if_index, &incoming_route);
	old_route = (struct rip_route_description *)rip_db_get(db, &incoming_route);

	if (old_route) {
		return handle_route_update(route_mngr, db, old_route, &incoming_route);
	} else {
		return handle_non_existing_route(route_mngr, db, &incoming_route);
	}
}

int rip_handle_response(struct rip_route_mngr *route_mngr, struct rip_db *db,
			struct rip2_entry entries[], size_t n_entry, struct in_addr sender_addr,
			int origin_if_index)
{
	int ret = 0;
	for (size_t i = 0; i < n_entry; ++i) {
		struct rip2_entry *entry = &entries[i];
		if (handle_ripv2_entry(route_mngr, db, entry, sender_addr, origin_if_index)) {
			ret = 1;
		}
	}

	return ret;
}

static int rip_recvfrom(int fd, struct msg_buffer *buff, struct in_addr *sender_address,
			size_t *bytes)
{
	struct sockaddr_in sa_in;
	MEMSET_ZERO(&sa_in);
	socklen_t sender_addr_len = sizeof(sa_in);

	ssize_t nbytes =
	    recvfrom(fd, buff, sizeof(*buff), 0, (struct sockaddr *)&sa_in, &sender_addr_len);
	if (nbytes < 0) {
		LOG_ERR("recvfrom failed: %s", strerror(errno));
		return 1;
	}

	sender_address->s_addr = sa_in.sin_addr.s_addr;
	*bytes		       = nbytes;

	return 0;
}

static size_t calculate_entries_count(size_t payload_size_bytes)
{
	return (payload_size_bytes - sizeof(struct rip_header)) / sizeof(struct rip2_entry);
}

static struct rip_socket *rip_find_rx_socket_by_fd(struct rip_ifc *ifcs, size_t entries_n,
						   const int fd)
{
	for (size_t i = 0; i < entries_n; ++i) {
		struct rip_ifc *ifc = &ifcs[i];
		if (ifc->socket_rx.fd == fd) {
			return &ifc->socket_rx;
		}
	}

	return NULL;
}

int rip_handle_message_event(const struct event *event)
{
	struct rip_context	*rip_ctx = event->arg;
	const struct rip_socket *socket =
	    rip_find_rx_socket_by_fd(rip_ctx->rip_ifcs, rip_ctx->rip_ifcs_n, event->fd);

	if (!socket) {
		BUG();
	}

	struct msg_buffer msg_buffer;
	struct in_addr	  sender;
	size_t		  n_bytes;
	if (rip_recvfrom(socket->fd, &msg_buffer, &sender, &n_bytes)) {
		return 1;
	}

	const size_t n_entries = calculate_entries_count(n_bytes);
	switch (msg_buffer.header.command) {
	case RIP_CMD_RESPONSE:
		return rip_handle_response(rip_ctx->route_mngr, &rip_ctx->rip_db,
					   msg_buffer.entries, n_entries, sender, socket->if_index);
	case RIP_CMD_REQUEST:
		return rip_send_advertisement_unicast(&rip_ctx->rip_db, msg_buffer.entries,
						      n_entries, sender, socket->if_index);
	default:
		LOG_ERR("Unsupported rip command: %d", msg_buffer.header.command);
		return 1;
	}
}
