#ifdef SHARED
#define pthread_mutex_timedlock __pthread_mutex_timedlock_nortm
#include "nptl/pthread_mutex_timedlock.c"
#endif
