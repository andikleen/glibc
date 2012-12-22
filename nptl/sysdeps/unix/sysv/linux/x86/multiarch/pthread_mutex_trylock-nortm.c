#define ENABLE_ELISION 0
#define __pthread_mutex_trylock __pthread_mutex_trylock_nortm
#include "nptl/pthread_mutex_trylock.c"
