/* The cond lock is not actually elided yet, but we still need to handle
   already elided locks and have a working ENABLE_ELISION. */
#include <elision-conf.h>
#define ENABLE_ELISION (__elision_available != 0)
#include "sysdeps/unix/sysv/linux/pthread_mutex_cond_lock.c"
