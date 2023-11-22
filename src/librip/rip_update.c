#include "rip_update.h"
#include "rip.h"
#include "utils/logging.h"
#include "utils/timer.h"

int rip_handle_timer_update(const struct rip_event *event)
{
    struct rip_context * ctx = event->arg1;
    if(timer_clear(&ctx->t_update)) {
        return 1;
    }
    
	LOG_INFO("rip_handle_timer_update");
	return 0;
}
