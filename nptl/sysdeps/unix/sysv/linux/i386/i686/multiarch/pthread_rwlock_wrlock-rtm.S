#define ENABLE_ELISION 1
#define __pthread_rwlock_wrlock __pthread_rwlock_wrlock_rtm
#include "sysdeps/unix/sysv/linux/i386/i686/pthread_rwlock_wrlock.S"
