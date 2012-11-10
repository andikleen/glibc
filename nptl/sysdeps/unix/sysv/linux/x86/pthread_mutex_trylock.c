#ifdef SHARED
#define __pthread_mutex_trylock __pthread_mutex_trylock_nortm
#include "nptl/pthread_mutex_trylock.c"
#endif
