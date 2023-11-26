#include "rip_update.h"
#include "rip.h"
#include "rip_common.h"
#include "utils/logging.h"
#include "utils/timer.h"
#include "utils/utils.h"
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <string.h>

int rip_send_advertisement(struct rip_context *ctx)
{
	(void)ctx;

	struct msg_buffer buffer;
	MEMSET_ZERO(&buffer);

	buffer.header.version = 2;
	buffer.header.command = RIP_CMD_RESPONSE;

	inet_aton("14.15.51.0", &buffer.entries[0].ip_address);
	inet_aton("0.0.0.0", &buffer.entries[0].next_hop);
	inet_aton("255.255.255.0", &buffer.entries[0].subnet_mask);
	buffer.entries[0].metric	    = 2;
	buffer.entries[0].routing_family_id = AF_INET;
	buffer.entries[0].route_tag	    = 100;
	rip2_entry_hton(&buffer.entries[0]);

	struct sockaddr_in socket_address;
	MEMSET_ZERO(&socket_address);
	socket_address.sin_family = AF_INET;
	socket_address.sin_port	  = htons(RIP_UDP_PORT);
	inet_aton(RIP_MULTICAST_ADDR, &socket_address.sin_addr);

	struct rip_socket *socket_tx = &ctx->rip_ifcs[0].socket_tx;
	ssize_t sent_bytes	     = sendto(socket_tx->fd, (char *)&buffer,
					      sizeof(struct rip_header) + sizeof(struct rip2_entry), 0,
					      (struct sockaddr *)&socket_address, sizeof(socket_address));

	if (sent_bytes == -1) {
		LOG_ERR("sendto failed: %s", strerror(errno));
		return 1;
	}

	LOG_INFO("send bytes: %zd", sent_bytes);
	return 0;
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
