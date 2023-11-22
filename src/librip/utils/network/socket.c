#include "utils/logging.h"
#include "utils/utils.h"

#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <errno.h>
#include <fcntl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

int socket_create_udp_socket(int *fd)
{
	int soc_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (soc_fd < 0) {
		LOG_ERR("socket failed: %s", strerror(errno));
		return 1;
	}
	*fd = soc_fd;
	return 0;
}

int socket_set_allow_reuse_port(int fd)
{
	if (fd <= 0)
		return 1;

	int yes = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, (char *)&yes,
		       sizeof(yes)) < 0) {
		LOG_ERR("SO_REUSEPORT failed (fd: %d): %s,", fd,
			strerror(errno));
		return 1;
	}

	return 0;
}

int socket_bind_port(int fd, int port)
{
	if (fd <= 0)
		return 1;

	struct sockaddr_in address;
	MEMSET_ZERO(&address);

	address.sin_family	= AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port	= htons(port);

	if (bind(fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
		LOG_ERR("bind failed (fd: %d): %s", fd, strerror(errno));
		return 1;
	}
	return 0;
}

int socket_join_multicast(int fd, int if_index, const char * address)
{
	if (fd <= 0)
		return 1;

	struct ip_mreqn mreq;
	MEMSET_ZERO(&mreq);

	mreq.imr_multiaddr.s_addr = inet_addr(address);
	mreq.imr_ifindex	  = if_index;

	if (setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&mreq,
		       sizeof(mreq)) < 0) {
		LOG_ERR("IP_ADD_MEMBERSHIP failed (fd: %d): %s", fd,
			strerror(errno));
		return 1;
	}

	return 0;
}

int socket_bind_to_device(int fd, const char if_name[IF_NAMESIZE])
{
	if (fd <= 0)
		return 1;

	if (if_name == NULL || if_name[0] == '\0') {
		LOG_ERR("if_name empty, fd: %d", fd);
		return 1;
	}

	if (setsockopt(fd, SOL_SOCKET, SO_BINDTODEVICE, if_name,
		       strnlen(if_name, IF_NAMESIZE)) < 0) {
		LOG_ERR("SO_BINDTODEVICE failed: %s", strerror(errno));
		return 1;
	}

	return 0;
}

int socket_set_nonblocking(int fd)
{
	if (fd <= 0)
		return 1;

	if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0) {
		LOG_ERR("fcntl F_SETFL,O_NONBLOCK failed : %s",
			strerror(errno));
		return 1;
	}

	return 0;
}
