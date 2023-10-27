#include "logging.h"
#include "rip_ipc.h"

#include <errno.h>
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

int main(int argc, char *argv[])
{
	int ret = 0;
	(void)argc;
	(void)argv;
	signal(SIGINT, sig_handler);

	struct rip_ipc *rip_ipc = rip_ipc_alloc();
	if (!rip_ipc) {
		return 1;
	}
	rip_ipc_init_cli(rip_ipc);

	struct ipc_request req	 = {.cmd = dump_routing_table};
	struct ipc_response resp = {0};

	rip_ipc_send_msg_cli(rip_ipc, req, &resp);

	if (resp.cmd_status == r_cmd_status_failed) {
		LOG_ERR("Failed to execute cmd\n");
		ret = 1;
	}
	printf("%s", resp.output);

	rip_ipc_free(rip_ipc);
	return ret;
}
