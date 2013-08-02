#include "sys/cdefs.h"
#include "sys/param.h"
#include "sys/kernel.h"
#include "sys/linker_set.h"
#include "kos/kos_kassert.h"

extern struct linker_set sysinit_set;

/**
 * This ensures that there is at least one entry so that the sysinit_set
 * symbol is not undefined. A subsystem ID of SI_SUB_DUMMY is never
 * executed.
 */
SYSINIT(placeholder, SI_SUB_DUMMY, SI_ORDER_ANY, NULL, NULL);

/**
 * The sysinit table itself. Items are checked off as they are run.
 * If we want to register new sysinit types, add them to newsysinit.
 */
SET_DECLARE(sysinit_set, struct sysinit);
struct sysinit **sysinit,     **sysinit_end;
struct sysinit **newsysinit,  **newsysinit_end;

void sysinit_startup() {
  register struct sysinit **sipp;   /* system initialization */
  register struct sysinit **xipp;   /* interior loop of sort */
  register struct sysinit *save;    /* used in bubble sort */

  if (sysinit == NULL) {
    sysinit = SET_BEGIN(sysinit_set);
    sysinit_end = SET_LIMIT(sysinit_set);
  }

  /**
   * Perform a bubble sort of the system initialization objects by
   * their subsystem (primary key) and order (secondary key).
   */
  for (sipp = sysinit; sipp < sysinit_end; sipp++) {
    for (xipp = sipp + 1; xipp < sysinit_end; xipp++) {
      if ((*sipp)->subsystem < (*xipp)->subsystem ||
          ((*sipp)->subsystem == (*xipp)->subsystem &&
           (*sipp)->order <= (*xipp)->order))
        continue;
      save = *sipp;
      *sipp = *xipp;
      *xipp = save;
    }
  }

  /**
   * Traverse the (now) ordered list of system initialization tasks.
   * Perform each task, and continue on to the next task.
   *
   * The last item on the list is expected to be the scheduler,
   * which will not return.
   *
   * BUT, I will not run scheduler here so it will return :)
   */
  for (sipp = sysinit; sipp < sysinit_end; sipp++) {
    if ((*sipp)->subsystem == SI_SUB_DUMMY) continue;
    if ((*sipp)->subsystem == SI_SUB_DONE) continue;
    (*((*sipp)->func))((*sipp)->udata);
    (*sipp)->subsystem = SI_SUB_DONE;     /* check off that it's done */
  }
}
