#ifndef UTILS_H
#define UTILS_H

#include <assert.h>
#include <string.h>

#define CALLOC(size) calloc(1, size)
#define MEMSET_ZERO(dest)                                                      \
	do                                                                     \
		memset(&dest, 0, sizeof(dest)); /*NOLINT */                    \
	while (0)

#define MIN(a, b) (a < b ? a : b)
#define ARRAY_LEN(array) (sizeof(array) / sizeof(array[0]))
#define BUG() assert(0 && "BUG")


#endif
