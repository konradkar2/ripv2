#ifndef RIP_RECV_H
#define RIP_RECV_H


#include <netinet/in.h>
#include <stdbool.h>
#include <stdint.h>
#include "utils/event.h"

int rip_handle_message_event(const struct event *event);
bool is_metric_valid(uint32_t metric);

#endif
