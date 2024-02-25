#ifndef UTILS_H
#define UTILS_H

#include "utils/logging.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define CALLOC(size) calloc(1, size)
#define MEMSET_ZERO(dest_p)                                                                        \
	do                                                                                         \
		memset(dest_p, 0, sizeof(*dest_p)); /*NOLINT */                                    \
	while (0)

#define MIN(a, b) (a < b ? a : b)
#define ARRAY_LEN(array) (sizeof(array) / sizeof(array[0]))
#define PANIC(status)                                                                              \
	do {                                                                                       \
		LOG_ERR("PANIC");                                                                  \
		exit(status);                                                                      \
	} while (0)

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define RIP_ASSERT(expr)                                                                           \
	do {                                                                                       \
		if (!(expr)) {                                                                     \
			LOG_ERR("assert failed: %s %s:%d", #expr, __FILENAME__, __LINE__);             \
			PANIC(1);                                                                  \
		}                                                                                  \
	} while (0)

float get_random_float(float min, float max);

#endif
