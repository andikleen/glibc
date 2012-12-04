/* Turn elision lock into timedlock */
#include "lowlevellock.h"
#include "force-elision.h"
#define EXTRAARG const struct timespec *timeout, 
#define LLL_LOCK(f,p) lll_timedlock(f, timeout, p)
#define __lll_lock_elision __lll_timedlock_elision
#include "elision-lock.c"
