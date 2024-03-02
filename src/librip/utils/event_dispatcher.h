#ifndef EVENT_DISPATCHER_H
#define EVENT_DISPATCHER_H

#include "event.h"
#include "hashmap.h"

struct pollfds_vec {
	struct pollfd *items;
	size_t	       count;
	size_t	       capacity;
};

struct event_dispatcher;

struct event_dispatcher *event_dispatcher_init(void);
void			 event_dispatcher_free(struct event_dispatcher *);

int event_dispatcher_register(struct event_dispatcher *, struct event *event);
int event_dispatcher_register_many(struct event_dispatcher *, struct event *events,
				   size_t events_len);

int event_dispatcher_poll_and_dispatch(struct event_dispatcher *);

#endif
