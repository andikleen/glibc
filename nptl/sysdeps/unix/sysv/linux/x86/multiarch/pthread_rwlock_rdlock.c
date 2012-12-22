#include "pthreadP.h"
#include "init-arch.h"
extern typeof(__pthread_rwlock_rdlock) __pthread_rwlock_rdlock_rtm;
extern typeof(__pthread_rwlock_rdlock) __pthread_rwlock_rdlock_nortm;
libm_ifunc(__pthread_rwlock_rdlock,
	   (HAS_RTM
	    ? __pthread_rwlock_rdlock_rtm
	    : __pthread_rwlock_rdlock_nortm))
strong_alias (__pthread_rwlock_rdlock, pthread_rwlock_rdlock)
libc_ifunc_hidden_def (__pthread_rwlock_rdlock)
