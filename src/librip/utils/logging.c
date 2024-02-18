#include "logging.h"
#include <time.h>

void get_time(char *buff, size_t buffer_size)
{
	struct tm *sTm;

	time_t now = time(NULL);
	sTm	   = gmtime(&now);

	strftime(buff, buffer_size, "%H:%M:%S", sTm);
}
