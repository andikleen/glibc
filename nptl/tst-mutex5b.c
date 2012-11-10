#define TYPE PTHREAD_MUTEX_TIMED_NP|PTHREAD_MUTEX_ELISION_NP
#include <elision-conf.h>
#ifdef HAVE_ELISION
#define USE_ELISION 1
#endif
#include "tst-mutex5.c"
