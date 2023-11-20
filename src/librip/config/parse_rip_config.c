#include "parse_rip_config.h"
#include "logging.h"
#include "utils.h"
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <yaml.h>

#define CHECK_NULL(objp)                                                                                               \
	do {                                                                                                           \
		if (objp == NULL) {                                                                                    \
			return 1;                                                                                      \
		}                                                                                                      \
	} while (0)

#define CHECK_TYPE(objp, yaml_expected_type)                                                                           \
	do {                                                                                                           \
		if (objp->type != yaml_expected_type) {                                                                \
			return 1;                                                                                      \
		}                                                                                                      \
	} while (0)

#define KEY_NAME_CMP(key, name) strncmp((const char *)key->data.scalar.value, name, strlen(name))

inline static size_t sequence_node_length(yaml_node_t *node)
{
	return node->data.sequence.items.top - node->data.sequence.items.start;
}

#define for_each_sequence_node_item(sequence_value, it)                                                                \
	for (it = sequence_value->data.sequence.items.start; it < sequence_value->data.sequence.items.top; ++it)

#define for_each_mapping_node_item(mapping_value, it)                                                                  \
	for (it = mapping_value->data.mapping.pairs.start; it < mapping_value->data.mapping.pairs.top; ++it)

inline static int str_to_int(const char *str, int *out)
{
	errno = 0;
	char *temp;
	long val = strtol(str, &temp, 0);

	if (temp == str || *temp != '\0' || ((val == LONG_MIN || val == LONG_MAX) && errno == ERANGE)) {
		LOG_ERR("Could not convert '%s' to long", str);
		return 1;
	}

	*out = val;
	return 0;
}

#define key_not_found 2

int extract_str_from_str_str_pair(yaml_document_t *document, yaml_node_pair_t *pair, const char *key_name, char **out)
{
	yaml_node_t *key   = NULL;
	yaml_node_t *value = NULL;
	key		   = yaml_document_get_node(document, pair->key);
	value		   = yaml_document_get_node(document, pair->value);

	CHECK_NULL(key);
	CHECK_NULL(value);
	CHECK_TYPE(key, YAML_SCALAR_NODE);
	CHECK_TYPE(value, YAML_SCALAR_NODE);
	if (KEY_NAME_CMP(key, key_name) == 0) {
		*out = strndup((const char *)value->data.scalar.value, value->data.scalar.length);
		return 0;
	}

	return key_not_found;
}

int extract_int_from_str_str_pair(yaml_document_t *document, yaml_node_pair_t *pair, const char *key_name, int **out)
{
	int ret = 0;
	char *str;

	int status = extract_str_from_str_str_pair(document, pair, key_name, &str);
	if (status) {
		return status;
	}

	int tmp = 0;
	if (str_to_int(str, &tmp)) {
		ret = 1;
		goto cleanup;
	}

	*out = malloc(sizeof(int));
	if (!out) {
		ret = 1;
		goto cleanup;
	}

	**out = tmp;
cleanup:
	free(str);
	return ret;
}

typedef int (*on_sequence_mapping_pair_cb)(yaml_document_t *document, int sequence_idx, yaml_node_pair_t *mapping_pair,
					   void *arg);
typedef int (*set_sequence_length_cb)(size_t sequence_length, void *arg1, void *arg2);
int parse_sequence_of_mappings(yaml_document_t *document, yaml_node_item_t item, set_sequence_length_cb cb_length,
			       on_sequence_mapping_pair_cb cb_on_sequence_pair, void *arg1, void *arg2)
{
	yaml_node_t *node = NULL;
	node		  = yaml_document_get_node(document, item);
	if (!node || node->type != YAML_SEQUENCE_NODE) {
		return 1;
	}

	size_t sequence_len = sequence_node_length(node);
	if (cb_length(sequence_len, arg1, arg2)) {
		return 1;
	}

	yaml_node_item_t *sequence_item;
	size_t seq_idx = 0;
	for_each_sequence_node_item(node, sequence_item)
	{
		yaml_node_t *sequence_node = NULL;
		sequence_node		   = yaml_document_get_node(document, *sequence_item);
		if (!sequence_node || sequence_node->type != YAML_MAPPING_NODE) {
			continue;
		}

		yaml_node_pair_t *sequence_mapping_pair = NULL;
		for_each_mapping_node_item(sequence_node, sequence_mapping_pair)
		{
			if (cb_on_sequence_pair(document, seq_idx, sequence_mapping_pair, arg1)) {
				return 1;
			}
		}
		++seq_idx;
	}

	return 0;
}

int parse_rip_interfaces_length(size_t sequence_length, void *arg1, void *arg2)
{
	struct rip_interface **ifcs = arg1;
	size_t *rip_interaces_n	    = arg2;

	*ifcs = CALLOC(sizeof(struct rip_interface) * sequence_length);
	if (*ifcs == NULL) {
		return 1;
	}

	*rip_interaces_n = sequence_length;

	return 0;
}

int parse_rip_interfaces_item(yaml_document_t *document, int sequence_idx, yaml_node_pair_t *mapping_pair, void *arg)
{
	struct rip_interface **ifcs    = arg;
	struct rip_interface *ifc_view = *ifcs;
	assert(ifc_view);

	struct rip_interface *ifc = &ifc_view[sequence_idx];

	if (extract_str_from_str_str_pair(document, mapping_pair, "dev", &ifc->dev) == 1) {
		return 1;
	};

	return 0;
}

int parse_rip_interfaces(yaml_document_t *document, yaml_node_item_t item, struct rip_interface **out,
			 size_t *rip_interaces_n)
{
	if (parse_sequence_of_mappings(document, item, parse_rip_interfaces_length, parse_rip_interfaces_item, out,
				       rip_interaces_n)) {
		return 1;
	}

	return 0;
}

int parse_rip_advertised_networks_length(size_t sequence_length, void *arg1, void *arg2)
{
	struct advertised_network **adv_nets = arg1;
	size_t *advertised_networks_n	     = arg2;

	*adv_nets = CALLOC(sizeof(struct advertised_network) * sequence_length);
	if (*adv_nets == NULL) {
		return 1;
	}

	*advertised_networks_n = sequence_length;

	return 0;
}

int parse_rip_advertised_networks_item(yaml_document_t *document, int sequence_idx, yaml_node_pair_t *mapping_pair,
				       void *arg)
{
	struct advertised_network **adv_nets	 = arg;
	struct advertised_network *adv_nets_view = *adv_nets;
	assert(adv_nets_view);

	struct advertised_network *net = &adv_nets_view[sequence_idx];

	if (extract_str_from_str_str_pair(document, mapping_pair, "dev", &net->dev) == 1) {
		return 1;
	};
	if (extract_int_from_str_str_pair(document, mapping_pair, "prefix", &net->prefix) == 1) {
		return 1;
	}
	if (extract_str_from_str_str_pair(document, mapping_pair, "address", &net->address) == 1) {
		return 1;
	}

	return 0;
}

int parse_rip_advertised_networks(yaml_document_t *document, yaml_node_item_t item, struct advertised_network **out,
				  size_t *advertised_networks_n)
{
	if (parse_sequence_of_mappings(document, item, parse_rip_advertised_networks_length,
				       parse_rip_advertised_networks_item, out, advertised_networks_n)) {
		return 1;
	}

	return 0;
}

int parse_rip_version(yaml_document_t *document, yaml_node_item_t item, int **version)
{
	yaml_node_t *value = NULL;
	*version	   = CALLOC(sizeof(int));

	if (!(*version))
		return 1;

	value = yaml_document_get_node(document, item);
	CHECK_NULL(value);
	if (str_to_int((const char *)value->data.scalar.value, *version)) {
		return 1;
	}
	return 0;
}

static int parse_rip_configuration(yaml_document_t *document, yaml_node_t *node, struct rip_configuration *rip_config)
{
	yaml_node_t *key, *value;
	yaml_node_pair_t *pair;

	MEMSET_ZERO(rip_config);

	pair = node->data.mapping.pairs.start;
	CHECK_NULL(pair);

	key   = yaml_document_get_node(document, pair->key);
	value = yaml_document_get_node(document, pair->value);

	CHECK_NULL(key);
	CHECK_NULL(value);
	CHECK_TYPE(key, YAML_SCALAR_NODE);
	CHECK_TYPE(value, YAML_MAPPING_NODE);

	if (0 != KEY_NAME_CMP(key, "rip_configuration")) {
		return 1;
	}

	yaml_node_pair_t *it;
	for_each_mapping_node_item(value, it)
	{
		yaml_node_t *key = NULL;
		key		 = yaml_document_get_node(document, it->key);
		if (!key)
			continue;

		if (key->type == YAML_SCALAR_NODE && KEY_NAME_CMP(key, "version") == 0) {
			if (parse_rip_version(document, it->value, &rip_config->version)) {
				return 1;
			}
		}

		if (key->type == YAML_SCALAR_NODE && KEY_NAME_CMP(key, "rip_interfaces") == 0) {
			if (parse_rip_interfaces(document, it->value, &rip_config->rip_interfaces,
						 &rip_config->rip_interfaces_n)) {
				return 1;
			}
		}
		if (key->type == YAML_SCALAR_NODE && KEY_NAME_CMP(key, "advertised_networks") == 0) {
			if (parse_rip_advertised_networks(document, it->value, &rip_config->advertised_networks,
							  &rip_config->advertised_networks_n)) {
				return 1;
			}
		}
	}
	return 0;
}

enum yaml_status { yaml_status_failed = 0, yaml_status_success = 1 };

int rip_configuration_validate(const struct rip_configuration *rip_config)
{
	CHECK_NULL(rip_config);
	CHECK_NULL(rip_config->version);
	CHECK_NULL(rip_config->rip_interfaces);
	for (size_t i = 0; i < rip_config->rip_interfaces_n; ++i) {
		struct rip_interface *ifc = &rip_config->rip_interfaces[i];
		CHECK_NULL(ifc->dev);
	}

	if (rip_config->advertised_networks_n > 0) {
		for (size_t i = 0; i < rip_config->advertised_networks_n; ++i) {
			struct advertised_network *net = &rip_config->advertised_networks[i];
			CHECK_NULL(net->dev);
			CHECK_NULL(net->prefix);
			CHECK_NULL(net->address);
		}
	}

	return 0;
}

void rip_configuration_print(const struct rip_configuration *rip_config)
{
#define SPACE " "
#define SPACE2 SPACE SPACE

	LOG_INFO("rip config:");
	LOG_INFO(SPACE "version: %d", *rip_config->version);
	LOG_INFO(SPACE "rip_interfaces:");
	for (size_t i = 0; i < rip_config->rip_interfaces_n; ++i) {
		struct rip_interface *ifc = &rip_config->rip_interfaces[i];
		LOG_INFO(SPACE2 "dev: %s", ifc->dev);
	}

	LOG_INFO(SPACE "advertised_networks:");
	if (rip_config->advertised_networks_n > 0) {
		for (size_t i = 0; i < rip_config->advertised_networks_n; ++i) {
			struct advertised_network *net = &rip_config->advertised_networks[i];
			LOG_INFO(SPACE2 "address: %s", net->address);
			LOG_INFO(SPACE2 "prefix: %d", *net->prefix);
			LOG_INFO(SPACE2 "dev: %s", net->dev);
		}
	}
}

int rip_configuration_read_and_parse(FILE *file, struct rip_configuration *rip_config)
{
	int ret = 0;
	assert(file);

	yaml_parser_t parser;
	yaml_document_t document;

	if (yaml_status_failed == yaml_parser_initialize(&parser)) {
		LOG_ERR("yaml_parser_initialize");
		ret = 1;
		goto cleanup;
	}
	yaml_parser_set_input_file(&parser, file);
	if (yaml_status_failed == yaml_parser_load(&parser, &document)) {
		LOG_ERR("yaml_parser_load");
		ret = 1;
		goto cleanup;
	}
	if (parse_rip_configuration(&document, yaml_document_get_root_node(&document), rip_config)) {
		ret = 1;
		goto cleanup;
	}

cleanup:
	yaml_document_delete(&document);
	yaml_parser_delete(&parser);

	return ret;
}

void rip_configuration_cleanup(struct rip_configuration *rip_config)
{
	if (!rip_config)
		return;

	free(rip_config->version);
	for (size_t i = 0; i < rip_config->advertised_networks_n; ++i) {
		free(rip_config->advertised_networks[i].address);
		free(rip_config->advertised_networks[i].dev);
		free(rip_config->advertised_networks[i].prefix);
	}
	free(rip_config->advertised_networks);

	for (size_t i = 0; i < rip_config->rip_interfaces_n; ++i) {
		free(rip_config->rip_interfaces[i].dev);
	}
	free(rip_config->rip_interfaces);
}
