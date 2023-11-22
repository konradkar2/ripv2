#include "utils/config/parse_rip_config.h"
#include "test.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

const char *yaml_content = "rip_configuration:\n"
			   "  version: 2\n"
			   "  rip_interfaces:\n"
			   "    - dev: eth0\n"
			   "    - dev: eth1\n"
			   "    - dev: eth3\n"
			   "  advertised_networks:\n"
			   "    - address: \"10.03.40.0\"\n"
			   "      prefix: 24\n"
			   "      dev: eth1\n"
			   "    - address: \"10.100.33.0\"\n"
			   "      prefix: 25\n"
			   "      dev: eth0\n";

REGISTER_TEST(parse_rip_config_test)
{
	FILE *file;
	file = fmemopen((void *)yaml_content, strlen(yaml_content), "r");
	ASSERT(file);

	struct rip_configuration cfg = {0};
	ASSERT(0 == rip_configuration_read_and_parse(file, &cfg));

	ASSERT(cfg.version);
	ASSERT(*cfg.version == 2);
	ASSERT(cfg.rip_interfaces_n == 3);
	ASSERT(cfg.rip_interfaces[0].dev);
	ASSERT(strcmp(cfg.rip_interfaces[0].dev, "eth0") == 0);
	ASSERT(cfg.rip_interfaces[1].dev);
	ASSERT(strcmp(cfg.rip_interfaces[1].dev, "eth1") == 0);
	ASSERT(cfg.rip_interfaces[2].dev);
	ASSERT(strcmp(cfg.rip_interfaces[2].dev, "eth3") == 0);

	ASSERT(cfg.advertised_networks_n == 2);
	ASSERT(cfg.advertised_networks[0].dev);
	ASSERT(cfg.advertised_networks[0].address);
	ASSERT(cfg.advertised_networks[0].prefix);
	ASSERT(strcmp(cfg.advertised_networks[0].dev, "eth1") == 0);
	ASSERT(*cfg.advertised_networks[0].prefix == 24);
	ASSERT(strcmp(cfg.advertised_networks[0].address, "10.03.40.0") == 0);

	ASSERT(cfg.advertised_networks[1].dev);
	ASSERT(cfg.advertised_networks[1].address);
	ASSERT(cfg.advertised_networks[1].prefix);
	ASSERT(strcmp(cfg.advertised_networks[1].dev, "eth0") == 0);
	ASSERT(*cfg.advertised_networks[1].prefix == 25);
	ASSERT(strcmp(cfg.advertised_networks[1].address, "10.100.33.0") == 0);

	ASSERT(rip_configuration_validate(&cfg) == 0);

	rip_configuration_cleanup(&cfg);
	fclose(file);
}
