#ifndef GLOBAL_H
#define GLOBAL_H

#include <stdio.h>

#define LOG_ERR(...)                                                           \
	do {                                                                   \
		fprintf(stderr, "ERROR: " __VA_ARGS__);                        \
		fprintf(stderr, "\n");                                         \
	} while (0)

#define LOG_INFO(...)                                                          \
	do {                                                                   \
		fprintf(stdin, "INFO: " __VA_ARGS__);                          \
		fprintf(stdin, "\n");                                          \
	} while (0)

#endif /* GLOBAL_H */