#ifndef RIP_IF_H
#define RIP_IF_H

#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/in.h>

int socket_create_udp_socket(int *fd);
int socket_set_allow_reuse_port(int fd);
int socket_bind_port(int fd, int port);
int socket_join_multicast(int fd, int if_index, const char * address);
int socket_bind_to_device(int fd, const char if_name[IF_NAMESIZE]);
int socket_set_nonblocking(int fd);

#endif
