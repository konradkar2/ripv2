#ifndef RIP_HANDLE_RESPONE_H
#define RIP_HANDLE_RESPONE_H

#include "rip.h"
#include "rip_messages.h"
#include <netinet/in.h>
#include <stddef.h>
int handle_response(rip_context *ctx, rip2_entry *entries, size_t n_entry,
		    struct in_addr sender);

#endif