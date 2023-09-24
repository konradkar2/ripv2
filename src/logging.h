#ifndef LOGGING_H
#define LOGGING_H

#include <stdio.h>

#define LOG_ERR(...)                                                           \
	do {                                                                   \
		fprintf(stderr, "ERROR: " __VA_ARGS__);                        \
		fprintf(stderr, "\n");                                         \
	} while (0)

#define LOG_INFO(...)                                                          \
	do {                                                                   \
		fprintf(stdout, "INFO: " __VA_ARGS__);                          \
		fprintf(stdout, "\n");                                          \
	} while (0)

#endif