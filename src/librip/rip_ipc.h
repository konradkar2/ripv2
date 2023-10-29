#ifndef RIP_IPC_H
#define RIP_IPC_H

#include <stddef.h>
#include <stdint.h>

enum rip_ipc_cmd {
	dump_routing_table = 0,
};

typedef int (*rip_ipc_cb)(char *resp_buffer, size_t buffer_size, void *data);

struct r_ipc_cmd_handler {
	enum rip_ipc_cmd cmd;
	rip_ipc_cb cb;
	void *data;
};

#define RESP_OUTPUT_SIZE 32 * 1024

enum r_cmd_status { r_cmd_status_success = 0, r_cmd_status_failed };
struct ipc_request {
	uint32_t cmd;
};
struct ipc_response {
	uint32_t cmd_status;
	char output[RESP_OUTPUT_SIZE - 4];
};

struct rip_ipc;

// Common functions
struct rip_ipc *rip_ipc_alloc(void);
void rip_ipc_free(struct rip_ipc *);
int rip_ipc_getfd(struct rip_ipc *);

// Deamons functions
int rip_ipc_init(struct rip_ipc *, struct r_ipc_cmd_handler handlers[],
		 size_t len);
void rip_ipc_handle_msg(struct rip_ipc *);

// CLI functions
void cli_rip_ipc_init(struct rip_ipc *);
void cli_rip_ipc_send_msg(struct rip_ipc *, struct ipc_request request,
			  struct ipc_response *resp);

#endif
