#define NAME "timed not elided"
#define USE_NO_ELISION 1
#define TYPE PTHREAD_MUTEX_TIMED_NP|PTHREAD_MUTEX_NO_ELISION_NP
#include "tst-mutex8.c"
