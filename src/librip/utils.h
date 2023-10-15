#ifndef UTILS_H
#define UTILS_H

#include <string.h>

#define MEMSET_ZERO(dest)                                                      \
	do                                                                     \
		memset(&dest, 0, sizeof(dest));                                \
	while (0)

#define MIN(a, b) (a < b ? a : b)

#endif
