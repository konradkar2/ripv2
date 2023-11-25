#ifndef EVENT_DISPATCHER_H
#define EVENT_DISPATCHER_H

#include "event.h"
#include "hashmap.h"

struct event_dispatcher {
	struct hashmap *events;
	struct vector *pollfds;
};

int event_dispatcher_init(struct event_dispatcher *);
void event_dispatcher_destroy(struct event_dispatcher *);
int event_dispatcher_register(struct event_dispatcher *, struct event *event);

int event_dispatcher_poll_and_dispatch(struct event_dispatcher *);

#endif
