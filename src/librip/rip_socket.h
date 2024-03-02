#ifndef RIP_SOCKET_H
#define RIP_SOCKET_H

#include "rip_common.h"
#include "utils/config/parse_rip_config.h"
#include <net/if.h>

int  rip_create_sockets(struct rip_configuration *cfg, struct rip_ifc_vec *rip_ifcs);
int  rip_setup_sockets(struct rip_ifc_vec *ifcs);
void rip_print_sockets(struct rip_ifc_vec *ifcs);
int  rip_create_socket_ifindex(struct rip_socket *socket, int ifindex);
int  rip_create_socket_ifname(struct rip_socket *socket, const char *ifname);
int  rip_setup_tx_socket(struct rip_socket *socket);

#endif
