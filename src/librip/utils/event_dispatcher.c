#include "event_dispatcher.h"
#include "utils/event.h"
#include "utils/hashmap.h"
#include "utils/logging.h"
#include "utils/utils.h"
#include <errno.h>
#include <poll.h>
#include <sys/poll.h>
#include "utils/da_array.h"

static int event_cmp(const void *el_a, const void *el_b, void *udata)
{
	(void)udata;
	const struct event *a = el_a;
	const struct event *b = el_b;

	return (a->fd > b->fd) - (a->fd < b->fd);
}

static uint64_t event_hash(const void *item, uint64_t seed0, uint64_t seed1)
{
	const struct event *event = item;
	return hashmap_sip(&event->fd, sizeof(event->fd), seed0, seed1);
}

int event_dispatcher_init(struct event_dispatcher *ed)
{
	MEMSET_ZERO(ed);
	struct hashmap **events_map = &ed->events;

	*events_map = hashmap_new(sizeof(struct event), 0, 0, 0, event_hash, event_cmp, NULL, NULL);
	if (!*events_map) {
		LOG_ERR("hashmap_new");
		return 1;
	}

	return 0;
}

int event_dispatcher_register(struct event_dispatcher *ed, struct event *e)
{
	struct hashmap *events_map  = ed->events;
	
	if (hashmap_get(events_map, e) != NULL) {
		LOG_ERR("event already registered, fd: %d", e->fd);
		return 1;
	}

	if (hashmap_set(events_map, e)) {
		LOG_ERR("hashmap_set, fd: %d", e->fd);
		return 1;
	}
	if (hashmap_oom(events_map) == true) {
		LOG_ERR("hashmap_oom");
		return 1;
	}

	struct pollfd pollfd = {.fd = e->fd, .events = POLLIN};
	da_append(&ed->pollfds, pollfd);
	if(!ed->pollfds.items)
	{
	 	LOG_ERR("vector_add, fd: %d", e->fd);
	 	return 1;
	}

	return 0;
}

int event_dispatcher_register_many(struct event_dispatcher *ed, struct event *events,
				   size_t events_len)
{
	for (size_t i = 0; i < events_len; ++i) {
		struct event *event = &events[i];
		if (event_dispatcher_register(ed, event)) {
			return 1;
		}
	}

	return 0;
}

void event_dispatcher_destroy(struct event_dispatcher *ed)
{
	if (!ed || !ed->events) {
		return;
	}

	hashmap_free(ed->events);
}

int event_dispatcher_poll_and_dispatch(struct event_dispatcher *ed)
{
	struct hashmap *events_map  = ed->events;
	struct pollfds_vec  *pollfds = &ed->pollfds;

	if (-1 == poll(pollfds->items, pollfds->count, -1)) {
		LOG_ERR("poll failed: %s", strerror(errno));
		return 1;
	}

	for (size_t i = 0; i < pollfds->count; ++i) {
		struct pollfd *pollfd = &pollfds->items[i];

		int revents = pollfd->revents;
		int fd	    = pollfd->fd;

		if (!(revents & POLLIN)) {
			continue;
		}

		const struct event *event = hashmap_get(events_map, &(struct event){.fd = fd});
		if (!event) {
			LOG_ERR("event not found, fd: %d", fd);
			return 1;
		}

		LOG_DEBUG("\"%s\" event on fd: %d", event->name, fd);
		event->cb(event);
	}

	return 0;
}
