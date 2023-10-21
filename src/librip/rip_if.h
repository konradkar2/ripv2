#ifndef RIP_IF_H
#define RIP_IF_H

#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/in.h>

typedef struct rip_if_entry {
	int fd;
	char if_name[IF_NAMESIZE];
	int if_index;
	char if_addr[INET_ADDRSTRLEN];
} rip_if_entry;

int create_udp_socket(int *fd);
int set_allow_reuse_port(int fd);
int bind_port(int fd);
int join_multicast(int fd, int if_index);
int bind_to_device(int fd, const char if_name[IF_NAMESIZE]);

#endif
