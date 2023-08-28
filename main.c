#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "global.h"

void sig_handler(int sig_num)
{
	LOG_ERR("%s: Got sig %d, exiting...", __func__, sig_num);
	exit(128 + sig_num);
}

int main()
{
    LOG_INFO("RIPV2 begin");
	signal(SIGINT, sig_handler);

    return 0;
}