#include <time.h>
#include "elision-conf.h"
#include "lowlevellock.h"
#define __lll_lock_elision __lll_timedlock_elision
#define EXTRAARG const struct timespec *t,
#undef LLL_LOCK
#define LLL_LOCK(a, b) lll_timedlock(a, t, b)
#include "elision-lock.c"
