#define NAME "timed elided"
#include <elision-conf.h>
#ifdef SUPPORTS_ELISION
#define ELIDED 1
#endif
#define TYPE PTHREAD_MUTEX_TIMED_NP|PTHREAD_MUTEX_ELISION_NP
#include "tst-mutex8.c"
