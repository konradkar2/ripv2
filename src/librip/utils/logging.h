#ifndef LOGGING_H
#define LOGGING_H

#include <stdio.h>

// e.g. 11:11:52
void get_time(char *buff, size_t size);

#define __LOG_FUNC__(format, loglevel, ...)                                                        \
	do {                                                                                       \
		char buff[20] = {0};                                                               \
		get_time(buff, sizeof(buff));                                                      \
		printf("%s %s %s:" format "\n", buff, loglevel, __func__, ##__VA_ARGS__);          \
		fflush(stdout);                                                                    \
	} while (0)

#define __LOG__(format, loglevel, ...)                                                             \
	do {                                                                                       \
		char buff[20] = {0};                                                               \
		get_time(buff, sizeof(buff));                                                      \
		printf("%s %s: " format "\n", buff, loglevel, ##__VA_ARGS__);                      \
		fflush(stdout);                                                                    \
	} while (0)

#define LOG_DEBUG(format, ...) __LOG_FUNC__(format, "DEBUG", ##__VA_ARGS__)
#define LOG_WARN(format, ...) __LOG_FUNC__(format, "WARN", ##__VA_ARGS__)
#define LOG_ERR(format, ...) __LOG_FUNC__(format, "ERROR", ##__VA_ARGS__)
#define LOG_INFO(format, ...) __LOG__(format, "INFO", ##__VA_ARGS__)

#endif
