#ifdef SHARED
#define __pthread_mutex_lock __pthread_mutex_lock_nortm
#include "nptl/pthread_mutex_lock.c"
#endif
