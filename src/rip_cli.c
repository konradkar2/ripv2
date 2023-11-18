#include "logging.h"
#include "rip_ipc.h"
#include <errno.h>
#include <getopt.h>
#include <net/if.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void sig_handler(int sig_num)
{
	LOG_ERR("%s: Got sig %d, exiting...", __func__, sig_num);
	exit(128 + sig_num);
}

int send_cmd(struct rip_ipc *rip_ipc, enum rip_ipc_cmd cmd)
{
	int ret = 0;

	struct ipc_request req	 = {.cmd = cmd};
	struct ipc_response resp = {0};

	cli_rip_ipc_send_msg(rip_ipc, req, &resp);

	if (resp.cmd_status == r_cmd_status_failed) {
		printf("Failed to execute cmd, r_cmd_status_failed\n");
		ret = 1;
	}
	
	printf("%s", resp.output);

	return ret;
}

void help(void)
{
	printf("Usage: rip-cli [OPTION]\n"
	       "\n"
	       "Options\n"
	       " -h, --help             Show this help\n"
	       " -r, --ripdump		Dump rip intermediate routes\n"
	       " -n, --nldump		Dump libnl's route cache (actual "
	       "kernel routes) \n");
}

int main(int argc, char *argv[])
{
	(void)argc;
	int ret = 0;
	signal(SIGINT, sig_handler);

	struct rip_ipc *rip_ipc = rip_ipc_alloc();
	if (!rip_ipc) {
		return 1;
	}
	cli_rip_ipc_init(rip_ipc);

	for (;;) {
		int c, optidx = 0;

		static struct option long_opts[] = {
		    {"help", 0, NULL, 'h'},
		    {"nldump", 0, NULL, 'n'},
		    {"ripdump", 0, NULL, 'r'},
		};

		c = getopt_long(argc, argv, "hnr", long_opts, &optidx);
		if (c == -1)
			break;

		switch (c) {
		case 'h':
			help();
			ret = 1;
			break;
		case 'n':
			ret = send_cmd(rip_ipc, dump_libnl_route_table);
			break;
		case 'r':
			ret = send_cmd(rip_ipc, dump_rip_routes);
			break;
		}
	}

	rip_ipc_free(rip_ipc);
	return ret;
}
