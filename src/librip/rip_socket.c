#include "rip_socket.h"
#include "utils/da_array.h"
#include "utils/logging.h"
#include "utils/network/socket.h"
#include "utils/utils.h"
#include <errno.h>
#include <net/if.h>
#include <string.h>

static void rip_socket_print(const struct rip_socket *e)
{
	printf("fd: %d, if_name: %s, if_index : %d", e->fd, e->if_name, e->if_index);
}

static int setup_socket_multicast_rx(struct rip_socket *sock)
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

static int setup_socket_multicast_tx(struct rip_socket *sock)
{
	if (rip_setup_tx_socket(sock)) {
		return 1;
	}
	if (socket_disable_multicast_loopback(sock->fd)) {
		return 1;
	}
	return 0;
}

int rip_create_socket_ifindex(struct rip_socket *socket, int ifindex)
{
	socket->if_index = ifindex;
	if (NULL == if_indextoname(socket->if_index, socket->if_name)) {
		LOG_ERR("if_indextoname : %s", strerror(errno));
		return 1;
	}

	if (socket_create_udp_socket(&socket->fd)) {
		return 1;
	}

	return 0;
}

int rip_setup_tx_socket(struct rip_socket *sock)
{
	if (socket_bind_to_device(sock->fd, sock->if_name)) {
		return 1;
	}
	if (socket_set_allow_reuse_port(sock->fd)) {
		return 1;
	}
	if (socket_bind_port(sock->fd, RIP_UDP_PORT)) {
		return 1;
	}

	return 0;
}

int rip_setup_sockets(struct rip_ifc_vec *ifcs)
{
	LOG_INFO("%s", __func__);

	for (size_t i = 0; i < ifcs->count; ++i) {
		struct rip_ifc *ifc = &ifcs->items[i];

		{
			struct rip_socket *socket_rx = &ifc->socket_rx;
			if (setup_socket_multicast_rx(socket_rx)) {
				LOG_ERR("setup_socket_rx");
				rip_socket_print(socket_rx);
				return 1;
			}
		}
		{
			struct rip_socket *socket_tx = &ifc->socket_tx;
			if (setup_socket_multicast_tx(socket_tx)) {
				LOG_ERR("setup_socket_tx");
				rip_socket_print(socket_tx);
				return 1;
			}
		}
	}

	return 0;
}

void rip_print_sockets(struct rip_ifc_vec *ifcs)
{
	LOG_INFO("%s", __func__);
	for (size_t i = 0; i < ifcs->count; ++i) {
		struct rip_ifc *ifc = &ifcs->items[i];
		printf("rx: ");
		rip_socket_print(&ifc->socket_rx);
		printf("\n");
		printf("tx: ");
		rip_socket_print(&ifc->socket_tx);
		printf("\n");
	}
}

int rip_create_socket_ifname(struct rip_socket *socket, const char *ifname)
{
	const size_t ifname_size = strlen(ifname);
	if (ifname_size == 0 || ifname_size > IF_NAMESIZE) {
		LOG_ERR("invalid dev name: %s", ifname);
		return 1;
	}

	strncpy(socket->if_name, ifname, ifname_size);
	socket->if_index = if_nametoindex(socket->if_name);
	if (socket->if_index == 0) {
		LOG_ERR("if_nametoindex failed (%s): %s", socket->if_name, strerror(errno));
		return 1;
	}

	if (socket_create_udp_socket(&socket->fd)) {
		return 1;
	}

	return 0;
}

int rip_create_sockets(struct rip_configuration *cfg, struct rip_ifc_vec *rip_ifcs)
{
	for (size_t i = 0; i < cfg->rip_interfaces_n; ++i) {
		struct rip_interface *interface_cfg = &cfg->rip_interfaces[i];

		struct rip_ifc ifc = {0};
		if (rip_create_socket_ifname(&ifc.socket_rx, interface_cfg->dev))
			return 1;
		if (rip_create_socket_ifname(&ifc.socket_tx, interface_cfg->dev))
			return 1;
		da_append(rip_ifcs, ifc);
		if(!rip_ifcs->items) {
			LOG_ERR("da_append");
		}
	}

	return 0;
}
