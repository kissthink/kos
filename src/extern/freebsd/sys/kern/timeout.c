#include "sys/param.h"
#include "sys/callout.h"
#include "sys/systm.h"
#include "kos/kos_dynamicTimer.h"

/**
 * Possible mpsafe values
 * 0 - not multiprocessor safe
 * CALLOUT_MPSAFE - multiprocessor safe
 */
void callout_init(struct callout *c, int mpsafe) {
  bzero(c, sizeof(*c));
}

/**
 * safe 0 - callout_stop
 * safe 1 - callout_drain
 */
int _callout_stop_safe(struct callout *c, int safe) {
  return kos_cancel_callout(c->c_func, c->c_arg, c->c_time, safe);
}

/**
 * New interface; clients allocate their own callout structures.
 *
 * callout_reset() - establish or change a timeout
 * callout_stop() - disestablish a timeout
 * callout_init() - initialize a callout structure so that it can
 * safely be passed to callout_reset() and callout_stop()
 *
 * <sys/callout.h> defines three convenience macros:
 *
 * callout_active() - returns truth if callout has not been stopped,
 *  drained, or deactivated since the last time the callout was
 *  reset.
 * callout_pending() - returns truth if callout is still waiting for timeout
 * callout_deactivate() - marks the callout as having been serviced
 */
int callout_reset_on(struct callout *c, int to_ticks, void (*ftn)(void *),
  void *arg, int cpu) {
  return kos_reset_callout(c->c_func, c->c_arg, c->c_time, to_ticks, ftn, arg);
}

int callout_schedule(struct callout *c, int to_ticks) {
  return callout_reset_on(c, to_ticks, c->c_func, c->c_arg, c->c_cpu);
}
