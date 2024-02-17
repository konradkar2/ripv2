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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <utils/network/socket.h>

void print_n_buffer(struct msg_buffer *buffer, size_t n_entries)
{
	for (size_t i = 0; i < n_entries; ++i) {
		rip2_entry_ntoh(&buffer->entries[i]);
		rip2_entry_print(&buffer->entries[i], stdout);
		printf("\n");
		rip2_entry_hton(&buffer->entries[i]);
	}
}

static int rip_send(int fd, struct in_addr destination, struct msg_buffer *buffer, size_t n_entries)
{
	if (n_entries == 0) {
		LOG_INFO("%s: 0 entries", __func__);
		return 0;
	}

	LOG_INFO("%s to %s, fd: %d", __func__, inet_ntoa(destination), fd);

	print_n_buffer(buffer, n_entries);

	struct sockaddr_in socket_address;
	MEMSET_ZERO(&socket_address);
	socket_address.sin_family = AF_INET;
	socket_address.sin_port	  = htons(RIP_UDP_PORT);
	socket_address.sin_addr	  = destination;

	size_t	send_size  = sizeof(struct rip_header) + sizeof(struct rip2_entry) * n_entries;
	ssize_t sent_bytes = sendto(fd, buffer, send_size, 0, (struct sockaddr *)&socket_address,
				    sizeof(socket_address));

	if (sent_bytes == -1) {
		LOG_ERR("sendto failed: %s", strerror(errno));
		return 1;
	}

	return 0;
}

enum rip_neighbour_policy { rip_neighbour_split_horizon, rip_neighbour_poison };

struct rip_advertising_policy {
	enum rip_neighbour_policy neigbour_policy;
	bool			  advertise_only_changed;
};

void fill_buffer_with_entries(uint32_t if_index_dest, struct rip_db *db, struct msg_buffer *buffer,
			      size_t *n_entries, const struct rip_advertising_policy policy)
{
	size_t				    buffer_entry_cnt = 0;
	size_t				    db_iter	     = 0;
	const struct rip_route_description *route;
	while (rip_db_iter(db, &db_iter, &route)) {

		if (policy.advertise_only_changed == true) {
			if (route->changed == false) {
				continue;
			}
		}

		struct rip2_entry *buffer_entry = &buffer->entries[buffer_entry_cnt];

		if (route->if_index != if_index_dest) {
			memcpy(buffer_entry, &route->entry, sizeof(struct rip2_entry));
			buffer_entry->next_hop.s_addr = 0;
			rip2_entry_hton(buffer_entry);
			++buffer_entry_cnt;
		} else {
			if (policy.neigbour_policy == rip_neighbour_split_horizon) {
				// skip
				continue;
			}
		}
	}

	*n_entries = buffer_entry_cnt;
}

int rip_send_response(struct msg_buffer *buffer, const struct rip_socket *socket,
		      struct in_addr destination, struct rip_db *db,
		      struct rip_advertising_policy policy)
{

	size_t n_entries = 0;
	fill_buffer_with_entries(socket->if_index, db, buffer, &n_entries, policy);
	rip_db_mark_all_routes_as_unchanged(db);
	return rip_send(socket->fd, destination, buffer, n_entries);
}

int rip_send_advertisement_multicast(struct rip_context *ctx, bool advertise_only_changed)
{
	int ret = 0;

	struct msg_buffer buffer;
	MEMSET_ZERO(&buffer);

	buffer.header.version = 2;
	buffer.header.command = RIP_CMD_RESPONSE;

	struct in_addr rip_address_n = {0};
	inet_aton(RIP_MULTICAST_ADDR, &rip_address_n);

	struct rip_advertising_policy policy = {.neigbour_policy = rip_neighbour_split_horizon,
						.advertise_only_changed = advertise_only_changed};

	for (size_t i = 0; i < ctx->rip_ifcs_n; ++i) {
		const struct rip_socket *socket = &ctx->rip_ifcs[i].socket_tx;
		ret |= rip_send_response(&buffer, socket, rip_address_n, &ctx->rip_db, policy);
	}

	return ret;
}

int rip_send_request_multicast(struct rip_context *ctx)
{
	int ret = 0;

	struct msg_buffer buffer;
	MEMSET_ZERO(&buffer);

	buffer.header.version = 2;
	buffer.header.command = RIP_CMD_REQUEST;
	size_t n_entries      = 1;
	buffer.entries[0]     = (struct rip2_entry){.metric = htonl(16)};

	struct in_addr rip_address_n = {0};
	inet_aton(RIP_MULTICAST_ADDR, &rip_address_n);

	LOG_INFO("Sending request command");
	for (size_t i = 0; i < ctx->rip_ifcs_n; ++i) {
		const struct rip_socket *socket = &ctx->rip_ifcs[i].socket_tx;
		ret |= rip_send(socket->fd, rip_address_n, &buffer, n_entries);
	}

	return ret;
}

static bool is_initial_request(struct rip2_entry entries[], size_t n_entry)
{
	return n_entry == 1 && entries[0].metric >= 16 && entries[0].routing_family_id == 0;
}

int rip_send_advertisement_unicast(struct rip_db *db, struct rip2_entry entries[], size_t n_entry,
				   struct in_addr sender_addr, int origin_if_index)
{
	LOG_INFO("%s to %s", __func__, inet_ntoa(sender_addr));

	if (n_entry == 0) {
		LOG_ERR("Invalid request from %s", inet_ntoa(sender_addr));
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

	struct rip_socket	      socket = {.fd = fd, .if_index = origin_if_index};
	struct rip_advertising_policy policy = {.neigbour_policy = rip_neighbour_split_horizon,
						.advertise_only_changed = false};

	int ret = rip_send_response(&buffer, &socket, sender_addr, db, policy);

	close(fd);
	return ret;
}
