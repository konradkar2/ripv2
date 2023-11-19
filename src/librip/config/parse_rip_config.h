#ifndef PARSE_RIP_CONFIG_H
#define PARSE_RIP_CONFIG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct rip_configuration {
	int *version;
	struct rip_interfaces {
		char *dev;
	} *rip_interfaces;
	size_t rip_interfaces_n;

	struct advertised_networks {
		char *address;
		char *prefix;
		char *dev;
	} *advertised_networks;
	size_t advertised_networks_n;
};

// caller should invoke fclose
int read_and_parse_rip_configuration(FILE *file, struct rip_configuration *rip_config);
void rip_configuration_cleanup(struct rip_configuration *rip_config);

#endif
