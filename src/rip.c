#include "rip.h"
#include "logging.h"
#include "rip_ifc.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>

#define RIP_PORT 520
#define MSGBUFSIZE 1024 * 32

#define MEMSET_ZERO(dest)                                                      \
	do                                                                     \
		memset(&dest, 0, sizeof(dest));                                \
	while (0)

static int create_socket(int *fd)
{
	int soc = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (soc < 0) {
		LOG_ERR("socket failed: %s", strerror(errno));
		return 1;
	}
	*fd = soc;
	return 0;
}

static int set_allow_reuse_addr(int fd)
{
	int yes = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&yes,
		       sizeof(yes)) < 0) {
		LOG_ERR("SO_REUSEADDR failed: %s", strerror(errno));
		return 1;
	}

	return 0;
};

static int bind_port(int fd)
{
	struct sockaddr_in address;
	MEMSET_ZERO(address);

	address.sin_family	= AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port	= htons(RIP_PORT);

	if (bind(fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
		LOG_ERR("bind failed: %s", strerror(errno));
		return 1;
	}
	return 0;
}

static int join_multicast(int fd)
{
	const char *rip_multicast_addr = "224.0.0.9";
	struct ip_mreq mreq;
	MEMSET_ZERO(mreq);

	mreq.imr_multiaddr.s_addr = inet_addr(rip_multicast_addr);
	mreq.imr_interface.s_addr = INADDR_ANY;

	if (setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&mreq,
		       sizeof(mreq)) < 0) {
		LOG_ERR("IP_ADD_MEMBERSHIP failed: %s", strerror(errno));
		return 1;
	}

	return 0;
}

static int setup_resources(rip_context *rip_ctx)
{
	LOG_INFO("%s", __func__);
	if (create_socket(&rip_ctx->fd)) {
		return 1;
	}
	if (set_allow_reuse_addr(rip_ctx->fd)) {
		return 1;
	}
	if (bind_port(rip_ctx->fd)) {
		return 1;
	}
	if (join_multicast(rip_ctx->fd)) {
		return 1;
	}

	return 0;
}

int rip_begin(rip_context *rip_ctx)
{
	LOG_INFO("%s", __func__);

	if (setup_resources(rip_ctx)) {
		LOG_ERR("failed to setup_resources");
		return 1;
	}

	LOG_INFO("Waiting for messages...");

	char buff[MSGBUFSIZE];
	struct sockaddr_in addr_n;
	while (1) {
		socklen_t addrlen = sizeof(addr_n);
		ssize_t nbytes	  = recvfrom(rip_ctx->fd, buff, MSGBUFSIZE, 0,
					     (struct sockaddr *)&addr_n, &addrlen);
		if (nbytes < 0) {
			LOG_ERR("recvfrom failed: %s", strerror(errno));
			return 1;
		}

		char addr_h[INET_ADDRSTRLEN];
		if (NULL != inet_ntop(AF_INET, &addr_n.sin_addr, addr_h,
				      INET_ADDRSTRLEN)) {
			LOG_INFO("Got message from %s", addr_h);
		} else {
			LOG_ERR("got message, but inet_ntop failed: %s",
				strerror(errno));
			continue;
		}

		char *buff_p = buff;
		rip_header_print((const rip_header *)buff_p);
		nbytes -= sizeof(rip_header);
		buff_p += sizeof(rip_header);

		size_t entry_count = nbytes / sizeof(rip2_entry);

		for (size_t i = 0; i < entry_count; ++i) {
			printf("[%zu]\n", i);
			rip2_entry_to_host((rip2_entry *)buff_p);
			rip2_entry_print((const rip2_entry *)buff_p);
			buff_p += sizeof(rip2_entry);
		}
	}

	

	return 0;
}