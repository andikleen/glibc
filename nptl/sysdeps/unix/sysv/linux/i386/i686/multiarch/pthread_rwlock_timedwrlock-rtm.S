#define ENABLE_ELISION 1
#define pthread_rwlock_timedwrlock __pthread_rwlock_timedwrlock_rtm
#include "sysdeps/unix/sysv/linux/i386/i686/pthread_rwlock_timedwrlock.S"
