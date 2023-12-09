#ifndef PARSE_RIP_CONFIG_H
#define PARSE_RIP_CONFIG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct rip_configuration {
	int *version;
	struct rip_interface {
		char *dev;
	} *rip_interfaces;
	size_t rip_interfaces_n;

	struct advertised_network {
		char *address;
		int *prefix;
		char *dev;
	} *advertised_networks;
	size_t advertised_networks_n;
};

int rip_read_config(const char * filename, struct rip_configuration *rip_cfg);
void rip_configuration_destroy(struct rip_configuration *rip_config);
void rip_configuration_print(const struct rip_configuration * rip_config);


//testing
// caller should close the file
int rip_configuration_read_and_parse(FILE *file, struct rip_configuration *rip_config);
int rip_configuration_validate(const struct rip_configuration * rip_config);

#endif
