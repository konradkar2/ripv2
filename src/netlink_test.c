
#include <arpa/inet.h>
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
	self_addr.nl_groups = RTMGRP_IPV4_ROUTE;

	if (bind(fd, (struct sockaddr *)&self_addr, sizeof(self_addr)) < 0) {
		errno_exit("bind");
	}

	return fd;
}

#define MAX_PAYLOAD 1024

void parse_rtattr(struct rtattr *tb[], int max, struct rtattr *rta, int len)
{
	// int i = 0;
	for (; RTA_OK(rta, len); rta = RTA_NEXT(rta, len)) {
		if (rta->rta_type <= max) {
			tb[rta->rta_type] = rta;
			// printf("parse_rtattr(%d): %d, len: %d\n", i,
			//       rta->rta_type, rta->rta_len);
			// i++;
		}
	}
}

void send_route_dump_request(int fd)
{
	struct rtmsg rhdr = {
	    .rtm_family = AF_INET // address family of routes or AF_UNSPEC
	};
	struct nlmsghdr *nlh = NULL;

	// char buffer [NLMSG_SPACE(MAX_PAYLOAD)] <- this is compile time;

	nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_PAYLOAD));
	if (!nlh) {
		printf("malloc");
		exit(1);
	}

	memset(nlh, 0, NLMSG_SPACE(MAX_PAYLOAD));
	nlh->nlmsg_pid	 = getpid(); // self pid
	nlh->nlmsg_len	 = NLMSG_SPACE(MAX_PAYLOAD);
	nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP; //| NLM_F_ACK;
	nlh->nlmsg_type	 = RTM_GETROUTE;
	nlh->nlmsg_seq += 1;

	memcpy(NLMSG_DATA(nlh), &rhdr, sizeof(rhdr));

	struct iovec iov = {.iov_base = (void *)nlh, .iov_len = nlh->nlmsg_len};

	struct sockaddr_nl dest_addr = {
	    .nl_family = AF_NETLINK,
	    .nl_pid    = 0, // kernel
	    .nl_groups = 0  // unicast
	};

	struct msghdr msg = {.msg_name	    = (void *)&dest_addr,
			     .msg_namelen   = sizeof(dest_addr),
			     .msg_iov	    = &iov,
			     msg.msg_iovlen = 1};

	int result = sendmsg(fd, &msg, 0);

	if (result < 0) {
		errno_exit("sendmsg");
	}

	printf("Waiting for message from kernel\n");

	int received_bytes = recvmsg(fd, &msg, 0);
	if (received_bytes < 0) // msg is also receiver for read
	{
		errno_exit("recvmsg");
	}

	printf("Received message, size: %d, sizeof rtmsg: %lu\n",
	       nlh->nlmsg_len, sizeof(struct rtmsg)); // msg -> iov -> nlh

	for (; NLMSG_OK(nlh, received_bytes);
	     nlh = NLMSG_NEXT(nlh, received_bytes)) {
		/* Get the route data */

		struct rtmsg *rtm = (struct rtmsg *)NLMSG_DATA(nlh);

		if (rtm->rtm_table != RT_TABLE_MAIN)
			return;
		struct rtattr *tb[RTA_MAX + 1] = {NULL};

		int len = nlh->nlmsg_len - NLMSG_LENGTH(sizeof(*rtm));
		parse_rtattr(tb, RTA_MAX, RTM_RTA(rtm), len);

		if (tb[RTA_DST]) {
			// Process destination attribute
			// Example: print the destination IP address
			struct in_addr *dst =
			    (struct in_addr *)RTA_DATA(tb[RTA_DST]);
			char dst_str[INET_ADDRSTRLEN];
			inet_ntop(AF_INET, dst, dst_str, INET_ADDRSTRLEN);
			printf("Destination: %s\n", dst_str);
		}
	}
}

int main(int argc, char **argv)
{
	(void)argc;
	(void)argv;
	printf("hello world\n");

	int fd = create_netlink_socket(NETLINK_ROUTE);
	printf("Got fd: %d!\n", fd);

	send_route_dump_request(fd);

	return 0;
}
