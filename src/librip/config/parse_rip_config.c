#include "parse_rip_config.h"
#include "logging.h"
#include "utils.h"
#include <assert.h>
#include <errno.h>
#include <limits.h>
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

int parse_rip_interfaces(yaml_document_t *document, yaml_node_item_t item, struct rip_interfaces **out,
			 size_t *rip_interaces_n)
{
	LOG_TRACE();
	yaml_node_t *node = NULL;
	node		  = yaml_document_get_node(document, item);
	if (node && node->type == YAML_SEQUENCE_NODE) {

		*rip_interaces_n = sequence_node_length(node);
		*out		 = CALLOC(*rip_interaces_n * sizeof(struct rip_interfaces));
		if (!(*out))
			return 1;

		struct rip_interfaces *out_it = *out;

		yaml_node_item_t *item_it;
		for_each_sequence_node_item(node, item_it)
		{
			yaml_node_t *mapping_node = NULL;
			mapping_node		  = yaml_document_get_node(document, *item_it);
			if (!mapping_node || mapping_node->type != YAML_MAPPING_NODE) {
				continue;
			}

			yaml_node_pair_t *mapping_pair = NULL;
			for_each_mapping_node_item(mapping_node, mapping_pair)
			{
				if (extract_str_from_str_str_pair(document, mapping_pair, "dev", &out_it->dev) == 1) {
					return 1;
				}
			}
			++out_it;
		}
	}

	return 0;
}

int parse_rip_advertised_networks(yaml_document_t *document, yaml_node_item_t item, struct advertised_networks **out,
				  size_t *advertised_networks_n)
{
	yaml_node_t *node = NULL;
	node		  = yaml_document_get_node(document, item);
	if (node && node->type == YAML_SEQUENCE_NODE) {

		*advertised_networks_n = sequence_node_length(node);
		*out		       = CALLOC(*advertised_networks_n * sizeof(struct advertised_networks));
		if (!(*out))
			return 1;

		struct advertised_networks *out_it = *out;

		yaml_node_item_t *item_it;
		for_each_sequence_node_item(node, item_it)
		{
			yaml_node_t *mapping_node = NULL;
			mapping_node		  = yaml_document_get_node(document, *item_it);
			if (!mapping_node || mapping_node->type != YAML_MAPPING_NODE) {
				continue;
			}

			yaml_node_pair_t *mapping_pair = NULL;
			for_each_mapping_node_item(mapping_node, mapping_pair)
			{
				if (extract_str_from_str_str_pair(document, mapping_pair, "dev", &out_it->dev) == 1) {
					return 1;
				};
				if (extract_str_from_str_str_pair(document, mapping_pair, "prefix", &out_it->prefix) ==
				    1) {
					return 1;
				}
				if (extract_str_from_str_str_pair(document, mapping_pair, "address",
								  &out_it->address) == 1) {
					return 1;
				}
			}
			++out_it;
		}
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
	printf("value->type: %d\n", value->type);
	if (str_to_int((const char *)value->data.scalar.value, *version)) {
		return 1;
	}
	return 0;
}

static int parse_rip_configuration(yaml_document_t *document, yaml_node_t *node, struct rip_configuration *rip_config)
{
	LOG_TRACE();
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

	if (rip_config->version)
		LOG_INFO("rip_config->version: %d", *rip_config->version);

	return 0;
}

enum yaml_status { yaml_status_failed = 0, yaml_status_success = 1 };

int read_and_parse_rip_configuration(FILE *file, struct rip_configuration *rip_config)
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
