#ifndef RIP_SOCKET_H
#define RIP_SOCKET_H

#include "rip_common.h"
#include "utils/config/parse_rip_config.h"

int rip_set_if_index_to_sockets(struct rip_configuration *cfg, struct rip_ifc **rip_ifcs,
				      size_t *rip_ifcs_n);
int rip_setup_sockets(struct rip_ifc *ifcs, size_t ifcs_n);


#endif
