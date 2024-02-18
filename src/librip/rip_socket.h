#ifndef RIP_SOCKET_H
#define RIP_SOCKET_H

#include "rip_common.h"
#include "utils/config/parse_rip_config.h"
#include <net/if.h>

int  rip_create_sockets(struct rip_configuration *cfg, struct rip_ifc **rip_ifcs,
				 size_t *rip_ifcs_n);
int  rip_setup_sockets(struct rip_ifc *ifcs, size_t ifcs_n);
void rip_print_sockets(struct rip_ifc *ifcs, size_t ifcs_n);
int  rip_create_socket_ifindex(struct rip_socket *socket, int ifindex);
int  rip_create_socket_ifname(struct rip_socket *socket, const char *ifname);
int  rip_setup_tx_socket(struct rip_socket * socket);

#endif
