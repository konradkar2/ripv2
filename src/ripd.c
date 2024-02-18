#include "rip.h"
#include "utils/logging.h"
#include "utils/utils.h"
#include <errno.h>
#include <execinfo.h>
#include <net/if.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void print_trace(void)
{
	void  *array[10];
	size_t size;

	size = backtrace(array, 10);
	backtrace_symbols_fd(array, size, STDERR_FILENO);
}

static struct rip_context *global_ctx;

void atexit_handler(void)
{
	if (global_ctx) {
		rip_cleanup(global_ctx);
	}
}

void sig_handler(int sig_num)
{
	LOG_INFO("%s: Got sig %s, exiting...", __func__, strsignal(sig_num));
	if (sig_num == SIGSEGV) {
		print_trace();
	}
	exit(128 + sig_num);
}

int main(int argc, char *argv[])
{
	(void)argc;
	(void)argv;
	signal(SIGSEGV, sig_handler);
	signal(SIGINT, sig_handler);
	signal(SIGTERM, sig_handler);

	atexit(atexit_handler);

	struct rip_context rip_ctx = {0};
	global_ctx		   = &rip_ctx;
	int ret			   = rip_begin(&rip_ctx);

	return ret;
}
