#ifndef UTILS_H
#define UTILS_H

#include <string.h>
#include <assert.h>

#define MEMSET_ZERO(dest)                                                      \
	do                                                                     \
		memset(&dest, 0, sizeof(dest));                                \
	while (0)

#define MIN(a, b) (a < b ? a : b)

#define ARRAY_LEN(array) (sizeof(array) / sizeof(array[0]))

#define BUG() assert(0 && "BUG")

#endif
