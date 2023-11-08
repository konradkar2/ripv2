
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define errno_exit(text)                                                       \
	do {                                                                   \
		perror(text);                                                  \
		exit(1);                                                       \
	} while (0)

int create_netlink_socket(int protocol)
{
	struct sockaddr_nl self_addr = {0};
	int flags		     = 0;

	int fd = socket(AF_NETLINK, SOCK_RAW | flags, protocol);
	if (fd < 0) {
		errno_exit("socket");
	}

	memset(&self_addr, 0, sizeof(self_addr));
	self_addr.nl_family = AF_NETLINK;
	self_addr.nl_pid    = getpid(); /* self pid */

	if (bind(fd, (struct sockaddr *)&self_addr, sizeof(self_addr)) < 0) {
		errno_exit("bind");
	}

	return fd;
}

#define MAX_PAYLOAD 1024

int send_route_dump_request(int fd)
{
	struct rtmsg rhdr = {
	    .rtm_family = AF_INET // address family of routes or AF_UNSPEC
	};

	struct nl_msg *msg;
	struct nlmsghdr *nlh = NULL;

	// char buffer [NLMSG_SPACE(MAX_PAYLOAD)] <- this is compile time;

	nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_PAYLOAD));
	memset(nlh, 0, NLMSG_SPACE(MAX_PAYLOAD));
	nlh->nlmsg_len	 = NLMSG_SPACE(MAX_PAYLOAD);
	nlh->nlmsg_pid	 = getpid(); // self pid
	nlh->nlmsg_flags = 0;
	nlh->nlmsg_type	 = RTM_GETROUTE;
	nlh->nlmsg_type	 = NLM_F_DUMP;

	memcpy(NLMSG_DATA(nlh), &rhdr, sizeof(rhdr));

	
}

int main(int argc, char **argv)
{
	(void)argc;
	(void)argv;
	printf("hello world\n");

	int fd = create_netlink_socket(NETLINK_ROUTE);
	printf("Got fd: %d!\n", fd);

	return 0;
}
