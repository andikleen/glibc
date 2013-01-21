#define TYPE PTHREAD_MUTEX_ADAPTIVE_NP|PTHREAD_MUTEX_ELISION_NP
#include <elision-conf.h>
#ifdef SUPPORTS_ELISION
#define ELIDED 1
#endif
#include "tst-mutex5.c"
