#define ENABLE_ELISION 1
#define __pthread_rwlock_rdlock __pthread_rwlock_rdlock_rtm
#include "sysdeps/unix/sysv/linux/x86_64/pthread_rwlock_rdlock.S"
