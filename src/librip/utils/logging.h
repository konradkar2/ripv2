#ifndef LOGGING_H
#define LOGGING_H

#include <stdio.h>

#define LOG_ERR(...)                                                                               \
	do {                                                                                       \
		fprintf(stdout, "ERROR %s: ", __func__);                                         \
		fprintf(stdout, __VA_ARGS__);                                                      \
		fprintf(stdout, "\n");                                                             \
		fflush(stdout);                                                                    \
	} while (0)

#define LOG_TRACE()                                                                                \
	do {                                                                                       \
		fprintf(stdout, "%s:%d", __FILE__, __LINE__);                                      \
		fprintf(stdout, "\n");                                                             \
		fflush(stdout);                                                                    \
	} while (0)

#define LOG_INFO(...)                                                                              \
	do {                                                                                       \
		fprintf(stdout, "INFO: " __VA_ARGS__);                                             \
		fprintf(stdout, "\n");                                                             \
		fflush(stdout);                                                                    \
	} while (0)

#endif
