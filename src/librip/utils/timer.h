#ifndef TIMER_H
#define TIMER_H

#include <stdbool.h>
struct timer {
	int fd;
	bool is_ticking;
};

int timer_init(struct timer *);
int timer_start_oneshot(struct timer *, float value_s);
int timer_clear(struct timer *);
bool timer_is_ticking(struct timer*);


#endif
