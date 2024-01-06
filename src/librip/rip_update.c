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
#include <utils/network/socket.h>

void print_n_buffer(struct msg_buffer *buffer, size_t n_entries)
{
	for (size_t i = 0; i < n_entries; ++i) {
		rip2_entry_ntoh(&buffer->entries[i]);
		rip2_entry_print(&buffer->entries[i], stdout);
		printf("\n");
		rip2_entry_hton(&buffer->entries[i]);
	}
	printf("\n");
}

static int rip_send(const struct rip_socket *socket_tx, struct in_addr destination,
		    struct msg_buffer *buffer, size_t n_entries)
{
	LOG_INFO("%s to %s", __func__, rip_ntop(destination));

	print_n_buffer(buffer, n_entries);

	struct sockaddr_in socket_address;
	MEMSET_ZERO(&socket_address);
	socket_address.sin_family = AF_INET;
	socket_address.sin_port	  = htons(RIP_UDP_PORT);
	socket_address.sin_addr	  = destination;

	size_t send_size   = sizeof(struct rip_header) + sizeof(struct rip2_entry) * n_entries;
	ssize_t sent_bytes = sendto(socket_tx->fd, buffer, send_size, 0,
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

void fill_buffer_with_entries(uint32_t if_index_dest, enum rip_policy policy, struct rip_db *db,
			      struct msg_buffer *buffer, size_t *n_entries)
{
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
}

int rip_send_adv(struct msg_buffer *buffer, const struct rip_socket *socket,
		 struct in_addr destination, enum rip_policy policy, struct rip_db *db)
{

	size_t n_entries = 0;
	fill_buffer_with_entries(socket->if_index, policy, db, buffer, &n_entries);
	return rip_send(socket, destination, buffer, n_entries);
}

int rip_send_advertisement(struct rip_context *ctx)
{
	int ret = 0;

	struct msg_buffer buffer;
	MEMSET_ZERO(&buffer);

	buffer.header.version = 2;
	buffer.header.command = RIP_CMD_RESPONSE;

	struct in_addr rip_address_n = {0};
	inet_aton(RIP_MULTICAST_ADDR, &rip_address_n);

	for (size_t i = 0; i < ctx->rip_ifcs_n; ++i) {
		const struct rip_socket *socket = &ctx->rip_ifcs[i].socket_tx;
		ret |= rip_send_adv(&buffer, socket, rip_address_n, rip_poliy_split_horizon,
				    &ctx->rip_db);
	}

	return ret;
}

static bool is_initial_request(struct rip2_entry entries[], size_t n_entry)
{
	if (n_entry == 1 && entries[0].metric >= 16 && entries[0].routing_family_id == 0) {
		return true;
	}
	return false;
}

int rip_send_advertisement_unicast(struct rip_db *db, struct rip2_entry entries[], size_t n_entry,
				   struct in_addr sender_addr, int origin_if_index)
{
	LOG_INFO("%s", __func__);

	if (n_entry == 0) {
		LOG_ERR("Invalid request from %s", rip_ntop(sender_addr));
		return 1;
	}

	rip2_entry_ntoh(&entries[0]);
	if (!is_initial_request(entries, n_entry)) {
		LOG_ERR("3.9.1 debug request not implemented");
		return 1;
	}

	struct msg_buffer buffer;
	MEMSET_ZERO(&buffer);

	buffer.header.version = 2;
	buffer.header.command = RIP_CMD_RESPONSE;

	int fd = 0;
	if (socket_create_udp_socket(&fd)) {
		LOG_ERR("socket_create_udp_socket");
		return 1;
	}

	struct rip_socket socket = {.fd = fd, .if_index = origin_if_index};
	return rip_send_adv(&buffer, &socket, sender_addr, rip_poliy_split_horizon, db);
}
