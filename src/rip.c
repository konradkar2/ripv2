#include "rip.h"
#include "logging.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>

static int create_socket(int *fd)
{
	int soc = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (soc < 0) {
		perror("socket failed");
		return 1;
	}
	*fd = soc;
	return 0;
}

static int set_allow_reuse_addr(int fd)
{
	u_int yes = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&yes,
		       sizeof(yes)) < 0) {
		perror("SO_REUSEADDR failed");
		return 1;
	}

	return 0;
};

int rip_begin(rip_context *rip_ctx)
{
	(void)rip_ctx;
	LOG_INFO("%s", __func__);

	int sock_fd;
	if (create_socket(&sock_fd))
		return 1;
	if (set_allow_reuse_addr(sock_fd))
		return 1;

	return 0;
}