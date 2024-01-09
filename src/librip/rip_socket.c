#include "rip_socket.h"
#include "utils/logging.h"
#include "utils/network/socket.h"
#include "utils/utils.h"
#include <errno.h>
#include <string.h>

static void rip_socket_print(const struct rip_socket *e)
{
	printf("fd: %d, if_name: %s, if_index : %d", e->fd, e->if_name, e->if_index);
}

static int setup_socket_rx(struct rip_socket *sock)
{
	if (socket_create_udp_socket(&sock->fd)) {
		return 1;
	}
	if (socket_set_nonblocking(sock->fd)) {
		return 1;
	}
	if (socket_bind_to_device(sock->fd, sock->if_name)) {
		return 1;
	}
	if (socket_set_allow_reuse_port(sock->fd)) {
		return 1;
	}
	if (socket_bind_port(sock->fd, RIP_UDP_PORT)) {
		return 1;
	}
	if (socket_join_multicast(sock->fd, sock->if_index, RIP_MULTICAST_ADDR)) {
		return 1;
	}

	return 0;
}

static int setup_socket_tx(struct rip_socket *sock)
{
	if (socket_create_udp_socket(&sock->fd)) {
		return 1;
	}
	if (socket_bind_to_device(sock->fd, sock->if_name)) {
		return 1;
	}
	if (socket_set_allow_reuse_port(sock->fd)) {
		return 1;
	}
	if (socket_bind_port(sock->fd, RIP_UDP_PORT)) {
		return 1;
	}
	if (socket_disable_multicast_loopback(sock->fd)) {
		return 1;
	}

	return 0;
}

int rip_setup_sockets(struct rip_ifc *ifcs, size_t ifcs_n)
{
	LOG_INFO("%s", __func__);

	for (size_t i = 0; i < ifcs_n; ++i) {
		struct rip_ifc *ifc = &ifcs[i];

		{
			struct rip_socket *socket_rx = &ifc->socket_rx;
			if (setup_socket_rx(socket_rx)) {
				LOG_ERR("setup_socket_rx");
				rip_socket_print(socket_rx);
				return 1;
			}
		}
		{
			struct rip_socket *socket_tx = &ifc->socket_tx;
			if (setup_socket_tx(socket_tx)) {
				LOG_ERR("setup_socket_tx");
				rip_socket_print(socket_tx);
				return 1;
			}
		}
	}

	return 0;
}

void rip_print_sockets(struct rip_ifc *ifcs, size_t ifcs_n)
{
	LOG_INFO("%s", __func__);
	for (size_t i = 0; i < ifcs_n; ++i) {
		struct rip_ifc *ifc = &ifcs[i];
		printf("rx: ");
		rip_socket_print(&ifc->socket_rx);
		printf("\n");
		printf("tx: ");
		rip_socket_print(&ifc->socket_tx);
		printf("\n");
	}
}

static int rip_assign_ifc_to_socket(struct rip_interface *interface_cfg, struct rip_socket *socket)
{
	strncpy(socket->if_name, interface_cfg->dev, sizeof(socket->if_name));
	if (strlen(socket->if_name) == 0) {
		LOG_ERR("if_name");
		return 1;
	}

	socket->if_index = if_nametoindex(socket->if_name);
	if (socket->if_index == 0) {
		LOG_ERR("if_nametoindex failed (%s): %s", socket->if_name, strerror(errno));
		return 1;
	}

	return 0;
}

int rip_set_if_index_to_sockets(struct rip_configuration *cfg, struct rip_ifc **rip_ifcs,
				size_t *rip_ifcs_n)
{
	*rip_ifcs = CALLOC(sizeof(struct rip_ifc) * cfg->rip_interfaces_n);
	if (!(*rip_ifcs)) {
		return 1;
	}
	*rip_ifcs_n = cfg->rip_interfaces_n;

	for (size_t i = 0; i < cfg->rip_interfaces_n; ++i) {
		struct rip_interface *interface_cfg = &cfg->rip_interfaces[i];
		struct rip_ifc *ifc		    = &(*rip_ifcs)[i];

		if (rip_assign_ifc_to_socket(interface_cfg, &ifc->socket_rx))
			return 1;
		if (rip_assign_ifc_to_socket(interface_cfg, &ifc->socket_tx))
			return 1;
	}

	return 0;
}
