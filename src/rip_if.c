#include "rip_if.h"
#include "logging.h"
#include "utils.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

#define RIP_MULTICAST_ADDR "224.0.0.9"
#define RIP_UDP_PORT 520

int create_udp_socket(int *fd)
{
	int soc_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (soc_fd < 0) {
		LOG_ERR("socket failed: %s", strerror(errno));
		return 1;
	}
	*fd = soc_fd;
	return 0;
}

int set_allow_reuse_addr(int fd)
{
	int yes = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&yes,
		       sizeof(yes)) < 0) {
		LOG_ERR("SO_REUSEADDR failed (fd: %d): %s,", fd,
			strerror(errno));
		return 1;
	}

	return 0;
};

int bind_port(int fd)
{
	struct sockaddr_in address;
	MEMSET_ZERO(address);

	address.sin_family	= AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port	= htons(RIP_UDP_PORT);

	if (bind(fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
		LOG_ERR("bind failed (fd: %d): %s", fd, strerror(errno));
		return 1;
	}
	return 0;
}

int join_multicast(int fd, int if_index, const char *local_ifc_address)
{
	struct ip_mreqn mreq;
	MEMSET_ZERO(mreq);

	mreq.imr_multiaddr.s_addr = inet_addr(RIP_MULTICAST_ADDR);
	mreq.imr_address.s_addr	  = inet_addr(local_ifc_address);
	mreq.imr_ifindex	  = if_index;

	if (setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&mreq,
		       sizeof(mreq)) < 0) {
		LOG_ERR("IP_ADD_MEMBERSHIP failed (fd: %d): %s", fd,
			strerror(errno));
		return 1;
	}

	return 0;
}