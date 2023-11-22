#include "utils/logging.h"
#include "rip.h"
#include "utils/utils.h"
#include <errno.h>
#include <execinfo.h>
#include <net/if.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void print_trace(void);

void sig_handler(int sig_num)
{
	LOG_ERR("%s: Got sig %s, exiting...", __func__, strsignal(sig_num));
	if (sig_num == SIGSEGV) {
		print_trace();
	}
	exit(128 + sig_num);
}

void print_trace(void)
{
	void *array[10];
	size_t size;

	size = backtrace(array, 10);
	backtrace_symbols_fd(array, size, STDERR_FILENO);
}

int main(int argc, char *argv[])
{
	(void)argc;
	(void)argv;
	signal(SIGINT, sig_handler);
	signal(SIGSEGV, sig_handler);

	struct rip_context rip_ctx = {0};
	int ret = rip_begin(&rip_ctx);

	return ret;
}
