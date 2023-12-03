#include "rip_common.h"
#include "test.h"
#include "rip.h"
#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>

REGISTER_TEST(rip_common_prefix_test)
{   
    const int prefix_len = 24;
    char buffer[INET_ADDRSTRLEN];

    struct in_addr result = {0};
    
    ASSERT(0 == prefix_len_to_subnet(prefix_len, &result));
    ASSERT(NULL != inet_ntop(AF_INET, &result, buffer, sizeof(buffer)));
    STR_EQ_N("255.255.255.0", buffer, 16);

    ASSERT(prefix_len == get_prefix_len(result));
}
