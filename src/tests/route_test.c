#include "rip_common.h"
#include "rip_route.h"
#include "test.h"
#include "utils/utils.h"
#include <arpa/inet.h>
#include <net/if.h>
#include <stdlib.h>
#include <unistd.h>

#define IFC_NAME "dummy1"
#define IFC_ADDRESS "10.1.1.1"
#define ROUTE_ADDRESS "10.100.1.0"
#define ROUTE_MASK "255.255.255.0"

int destroy_env(void)
{
	int ret = 0;

	ret |= system("ip link set up down " IFC_NAME);
	ret |= system("ip link delete " IFC_NAME " type dummy");

	return ret;
}

int init_env(void)
{
	destroy_env();
	int ret = 0;
	ret |= system("ip link add " IFC_NAME " type dummy");
	ret |= system("ip link set up dev " IFC_NAME);
	ret |= system("ip address add " IFC_ADDRESS "/24 dev " IFC_NAME);

	return ret;
}

REGISTER_TEST(rip_route_test_add_delete)
{
	ASSERT(init_env() == 0);
	struct rip_route_mngr *mngr = NULL;
	mngr			    = rip_route_alloc_init();
	assert(mngr);

	struct rip_route_description route;
	MEMSET_ZERO(&route);

	route.if_index = if_nametoindex(IFC_NAME);
	ASSERT(route.if_index != 0);
	ASSERT(1 == inet_pton(AF_INET, ROUTE_ADDRESS, &route.entry.ip_address.s_addr));
	ASSERT(1 == inet_pton(AF_INET, ROUTE_MASK, &route.entry.subnet_mask.s_addr));

	ASSERT(0 == rip_route_add_route(mngr, &route));
	ASSERT(0 ==
	       system("ip route show | grep -q \"10.100.1.0/24 dev dummy1 proto rip metric 20\""));
	ASSERT(0 == rip_route_delete_route(mngr, &route));
	ASSERT(0 !=
	       system("ip route show | grep -q \"10.100.1.0/24 dev dummy1 proto rip metric 20\""));

	rip_route_free(mngr);
	ASSERT(destroy_env() == 0);
}
