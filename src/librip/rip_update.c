#include "rip_update.h"
#include "rip.h"
#include "utils/logging.h"
#include "utils/timer.h"

int rip_send_advertisement(struct rip_context *ctx) {
    (void)ctx;

    return 0;
}

int rip_handle_t_update(const struct event *event)
{
    LOG_INFO("rip_handle_timer_update");

	struct rip_context *ctx = event->arg;
	if (timer_clear(&ctx->t_update)) {
		return 1;
	}

	return rip_send_advertisement(ctx);
}
