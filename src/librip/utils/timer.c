#include "timer.h"
#include "utils/logging.h"
#include <bits/time.h>
#include <bits/types/struct_itimerspec.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <sys/timerfd.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <poll.h>

int timer_init(struct timer *timer)
{
	if (timer->fd != 0) {
		return 1;
	}

	int fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
	if (fd == -1) {
		LOG_ERR("timerfd_create: %s", strerror(errno));
		return 1;
	}

	timer->fd = fd;
	return 0;
}

static int timer_set(struct timer *t, struct itimerspec *tspec)
{
	if (timerfd_settime(t->fd, 0, tspec, NULL)) {
		LOG_ERR("timerfd_settime: %s", strerror(errno));
		return 1;
	}

	return 0;
}

int timer_start_oneshot(struct timer *t, float value_s)
{
	int seconds	     = value_s;
	long long nanosecond = (value_s - seconds) * 1000.0f * 1000.0f * 1000.0f;

	struct itimerspec timerspec = (struct itimerspec){
	    .it_interval = {0, 0}, .it_value = {.tv_sec = (int)value_s, .tv_nsec = nanosecond}};

	return timer_set(t, &timerspec);
}

int timer_clear(struct timer *t)
{
	uint64_t buff;
	if (-1 == read(t->fd, &buff, sizeof(buff))) {
		LOG_ERR("read, fd: %d, %s", t->fd, strerror(errno));
		return 1;
	}

	return 0;
}



