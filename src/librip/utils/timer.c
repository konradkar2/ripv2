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

int timer_start_interval(struct timer *t, int interval_s, int value_s)
{
	struct timespec interval = (struct timespec){.tv_sec = interval_s};
	struct timespec value	 = (struct timespec){.tv_sec = value_s};
	struct itimerspec timerspec =
	    (struct itimerspec){.it_interval = interval, .it_value = value};

	if (timerfd_settime(t->fd, 0, &timerspec, NULL)) {
		LOG_ERR("timerfd_settime: %s", strerror(errno));
		return 1;
	}

	return 0;
}

int timer_clear(struct timer *t)
{
	uint64_t buff;
	if (-1 == read(t->fd, &buff, sizeof(buff))) {
		LOG_ERR("read failed, fd: %d, %s", t->fd, strerror(errno));
		return 1;
	}

	return 0;
}
