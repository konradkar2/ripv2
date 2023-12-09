#include "test.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <rip_recv.h>
#include <stdint.h>

struct in_addr my_pton(const char *addr_p)
{
	struct in_addr addr = {0};
	int res		    = inet_pton(AF_INET, addr_p, &addr);
	assert(res == 1);

	return addr;
}

REGISTER_TEST(valid_mask_test)
{
	ASSERT(is_net_mask_valid(my_pton("255.255.255.0")));
	ASSERT(is_net_mask_valid(my_pton("255.255.255.254")));
	ASSERT(false == is_net_mask_valid(my_pton("128.255.255.0")));
	ASSERT(false == is_net_mask_valid(my_pton("254.255.255.0")));

	ASSERT(false == is_net_mask_valid(my_pton("255.255.255.255")));
	ASSERT(false == is_net_mask_valid(my_pton("0.255.255.0")));
}

REGISTER_TEST(is_unicast_address)
{

	ASSERT(is_unicast_address(my_pton("10.0.0.1")));
	ASSERT(is_unicast_address(my_pton("123.52.52.3")));

	ASSERT(false == is_unicast_address(my_pton("127.0.0.0")));
	ASSERT(false == is_unicast_address(my_pton("0.0.0.0")));
	ASSERT(false == is_unicast_address(my_pton("224.0.0.0")));
	ASSERT(false == is_unicast_address(my_pton("245.0.0.5")));
}
