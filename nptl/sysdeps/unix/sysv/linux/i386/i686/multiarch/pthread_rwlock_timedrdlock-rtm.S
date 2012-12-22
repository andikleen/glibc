#define ENABLE_ELISION 1
#define pthread_rwlock_timedrdlock __pthread_rwlock_timedrdlock_rtm
#include "sysdeps/unix/sysv/linux/i386/i686/pthread_rwlock_timedrdlock.S"
