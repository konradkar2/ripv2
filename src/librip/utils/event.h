#ifndef EVENT_H
#define EVENT_H

struct event;
typedef int (*event_cb)(const struct event *);

struct event {
	int fd;
	event_cb cb;
	void *arg;
	const char * name;
};

#endif
