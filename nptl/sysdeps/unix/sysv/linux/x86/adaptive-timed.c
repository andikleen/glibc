/* Turn adaptive lock into timedlock */
#include "lowlevellock.h"
#define EXTRAARG const struct timespec *timeout, 
#define LLL_LOCK(f,p) lll_timedlock(f, timeout, p)
#define __lll_adaptive_lock_hle __lll_timedlock_adaptive_hle
#include "adaptive-lock.c"
