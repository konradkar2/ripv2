#include "test.h"
#include <netinet/in.h>
#include <rip_handle_resp.h>

REGISTER_TEST(valid_mask_test)
{
	struct in_addr addr = {0};
	inet_pton(AF_INET, "255.255.255.0", &addr);
	ASSERT(is_net_mask_valid(addr));

	inet_pton(AF_INET, "255.255.255.255", &addr);
	ASSERT(is_net_mask_valid(addr));

	inet_pton(AF_INET, "128.0.0.0", &addr);
	ASSERT(is_net_mask_valid(addr));

	inet_pton(AF_INET, "0.255.255.0", &addr);
	ASSERT(false == is_net_mask_valid(addr));
}

REGISTER_TEST(is_unicast_address)
{
	struct in_addr addr = {0};
	inet_pton(AF_INET, "10.0.0.1", &addr);
	ASSERT(is_unicast_address(addr));

	inet_pton(AF_INET, "123.52.52.3", &addr);
	ASSERT(is_unicast_address(addr));

	inet_pton(AF_INET, "127.0.0.0", &addr);
	ASSERT(false == is_unicast_address(addr));

	inet_pton(AF_INET, "0.0.0.0", &addr);
	ASSERT(false == is_unicast_address(addr));

	inet_pton(AF_INET, "224.0.0.0", &addr);
	ASSERT(false == is_unicast_address(addr));
}