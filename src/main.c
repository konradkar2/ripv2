#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "logging.h"

void sig_handler(int sig_num)
{
	LOG_ERR("%s: Got sig %d, exiting...", __func__, sig_num);
	exit(128 + sig_num);
}

int main(int argc, char *argv[])
{
	(void)argc;
	(void)argv;
	LOG_INFO("RIPV2 begin");

	return 0;
}