#include "logging.h"
#include "rip.h"
#include "rip_if.h"
#include "utils.h"

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

	struct rip_context rip_ctx = {.rip_ifs_count = 2,
				      .rip_ifs	     = {[0] = {.if_name = "eth0"}, [1] = {.if_name = "eth1"}}};

	// temporary here, to be de done after some config parsing
	for (size_t i = 0; i < rip_ctx.rip_ifs_count; ++i) {
		rip_if_entry *entry = &rip_ctx.rip_ifs[i];
		if (entry->if_name[0] != '\0') {
			entry->if_index = if_nametoindex(entry->if_name);
			if (entry->if_index == 0) {
				LOG_ERR("if_nametoindex failed (%s): %s", entry->if_name, strerror(errno));
				return 1;
			}
		}
	}

	int ret = rip_begin(&rip_ctx);

	return ret;
}
