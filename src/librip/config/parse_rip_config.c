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

inline static int str_to_int(const char *str, int *out)
{
	errno = 0;
	char *temp;
	long val = strtol(str, &temp, 0);

	if (temp == str || *temp != '\0' || ((val == LONG_MIN || val == LONG_MAX) && errno == ERANGE)) {
		LOG_ERR("Could not convert '%s' to long and leftover string is: '%s'\n", str, temp);
		return 1;
	}

	*out = val;
	return 0;
}

int parse_rip_version(yaml_document_t *document, yaml_node_pair_t *start, yaml_node_pair_t *end, int *version)
{
	for (yaml_node_pair_t *it = start; it != end; ++it) {
		yaml_node_t *key, *value;
		key = yaml_document_get_node(document, it->key);
		if (key && key->type == YAML_SCALAR_NODE && KEY_NAME_CMP(key, "version") == 0) {
			value = yaml_document_get_node(document, it->value);
			CHECK_NULL(value);

			if (str_to_int((const char *)value->data.scalar.value, version)) {
				return 1;
			}

			return 0;
		}
	}

	return 1;
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

	if (0 != strncmp((const char *)key->data.scalar.value, "rip_configuration", strlen("rip_configuration"))) {
		return 1;
	}

	if (parse_rip_version(document, value->data.mapping.pairs.start, value->data.mapping.pairs.end,
			      &rip_config->version)) {
		return 1;
	}

	LOG_INFO("rip_config->version: %d", rip_config->version);

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
	yaml_parser_delete(&parser);

	return ret;
}
