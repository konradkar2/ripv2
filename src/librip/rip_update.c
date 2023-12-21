#include "rip_update.h"
#include "rip.h"
#include "rip_common.h"
#include "rip_db.h"
#include "utils/logging.h"
#include "utils/timer.h"
#include "utils/utils.h"
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <string.h>

static int rip_send(const struct rip_socket *socket_tx, char *buffer, size_t buffer_size)
{
	struct sockaddr_in socket_address;
	MEMSET_ZERO(&socket_address);
	socket_address.sin_family = AF_INET;
	socket_address.sin_port	  = htons(RIP_UDP_PORT);
	inet_aton(RIP_MULTICAST_ADDR, &socket_address.sin_addr);

	ssize_t sent_bytes = sendto(socket_tx->fd, buffer, buffer_size, 0,
				    (struct sockaddr *)&socket_address, sizeof(socket_address));

	if (sent_bytes == -1) {
		LOG_ERR("sendto failed: %s", strerror(errno));
		return 1;
	}
	LOG_INFO("send bytes: %zd", sent_bytes);

	return 0;
}

enum rip_policy {
	rip_poliy_split_horizon,
};

int fill_buffer_with_entries(uint32_t if_index_dest, enum rip_policy policy, struct rip_db *db,
			     struct msg_buffer *buffer, size_t *n_entries)
{
	LOG_INFO("fill_buffer_with_entries");

	size_t written_entries_n = 0;
	size_t db_iter		 = 0;
	struct rip_route_description *db_route;
	while (rip_db_iter(db, &db_iter, &db_route)) {
		struct rip2_entry *entry_out = &buffer->entries[written_entries_n];

		if (db_route->if_index != if_index_dest) {
			memcpy(entry_out, &db_route->entry, sizeof(struct rip2_entry));
			rip2_entry_hton(entry_out);
			++written_entries_n;
		} else {
			if (policy == rip_poliy_split_horizon) {
				// skip
				continue;
			}
		}
	}

	*n_entries = written_entries_n;

	return 0;
}

int rip_send_advertisement(struct rip_context *ctx)
{
	int ret = 0;

	struct msg_buffer buffer;
	MEMSET_ZERO(&buffer);

	buffer.header.version = 2;
	buffer.header.command = RIP_CMD_RESPONSE;

	for (size_t i = 0; i < ctx->rip_ifcs_n; ++i) {
		const struct rip_socket *socket_tx = &ctx->rip_ifcs[i].socket_tx;
		size_t n_entries		   = 0;

		fill_buffer_with_entries(socket_tx->if_index, rip_poliy_split_horizon, &ctx->rip_db,
					 &buffer, &n_entries);
		ret |= rip_send(socket_tx, (char *)&buffer,
				sizeof(struct rip_header) + sizeof(struct rip2_entry) * n_entries);
	}

	return ret;
}

int rip_handle_t_update(const struct event *event)
{
	LOG_INFO("rip_handle_timer_update");

	struct rip_context *ctx = event->arg;
	if (timer_clear(&ctx->t_update)) {
		return 1;
	}

	return rip_send_advertisement(ctx);
}
