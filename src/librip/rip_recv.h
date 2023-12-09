#ifndef RIP_RECV_H
#define RIP_RECV_H

#include "rip.h"
#include "rip_common.h"
#include <netinet/in.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

int rip_handle_message_event(const struct event *event);
bool is_metric_valid(uint32_t metric);

#endif
