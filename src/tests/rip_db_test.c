#include "rip_common.h"
#include "rip_db.h"
#include "test.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

REGISTER_TEST(rip_db_add_test)
{
	struct rip_db *db = rip_db_init();
	ASSERT(db);
	
	struct rip_route_description el = {.if_index = 5,
					   .entry    = {
						  .ip_address.s_addr  = 1234,
						  .next_hop.s_addr    = 2345,
						  .subnet_mask.s_addr = 3456,
					      }};

	ASSERT(rip_db_add(db, &el) == 0);
	ASSERT(rip_db_get(db, &el) != NULL);
	ASSERT(rip_db_add(db, &el) == 1);

	{
		struct rip_route_description el_if_index = el;
		++el_if_index.if_index;
		ASSERT(rip_db_add(db, &el_if_index) == 0);
	}
	{
		struct rip_route_description el_ip = el;
		++el_ip.entry.ip_address.s_addr;
		ASSERT(rip_db_add(db, &el_ip) == 0);
		ASSERT(rip_db_add(db, &el_ip) == 1);
	}
	{
		struct rip_route_description el_subnet = el;
		++el_subnet.entry.subnet_mask.s_addr;
		ASSERT(rip_db_add(db, &el_subnet) == 0);
		ASSERT(rip_db_add(db, &el_subnet) == 1);
	}
	{
		struct rip_route_description el_nexthop = el;
		++el_nexthop.entry.next_hop.s_addr;
		ASSERT(rip_db_add(db, &el_nexthop) == 0);
		ASSERT(rip_db_add(db, &el_nexthop) == 1);
	}

	{
		struct rip_route_description el_if_index = el;
		--el_if_index.if_index;
		ASSERT(rip_db_add(db, &el_if_index) == 0);
	}
	{
		struct rip_route_description el_ip = el;
		--el_ip.entry.ip_address.s_addr;
		ASSERT(rip_db_add(db, &el_ip) == 0);
		ASSERT(rip_db_add(db, &el_ip) == 1);
	}
	{
		struct rip_route_description el_subnet = el;
		--el_subnet.entry.subnet_mask.s_addr;
		ASSERT(rip_db_add(db, &el_subnet) == 0);
		ASSERT(rip_db_add(db, &el_subnet) == 1);
	}
	{
		struct rip_route_description el_nexthop = el;
		--el_nexthop.entry.next_hop.s_addr;
		ASSERT(rip_db_add(db, &el_nexthop) == 0);
		ASSERT(rip_db_add(db, &el_nexthop) == 1);
	}
	rip_db_free(db);
}

REGISTER_TEST(rip_db_iter_test)
{
	struct rip_db *db = rip_db_init();
	ASSERT(db);
	
	struct rip_route_description el = {.if_index = 5,
					   .entry    = {
						  .ip_address.s_addr  = 1234,
						  .next_hop.s_addr    = 2345,
						  .subnet_mask.s_addr = 3456,
					      }};

	ASSERT(rip_db_add(db, &el) == 0);
	{
		struct rip_route_description element = el;
		++element.if_index;
		ASSERT(rip_db_add(db, &element) == 0);
	}

	struct rip_db_iter		    iter	    = {0};
	const struct rip_route_description *current_element = NULL;
	int				    count	    = 0;
	while (rip_db_iter(db, &iter, &current_element)) {
		ASSERT(current_element != NULL);
		count++;
	}
	ASSERT(count == 2);

	rip_db_free(db);
}
