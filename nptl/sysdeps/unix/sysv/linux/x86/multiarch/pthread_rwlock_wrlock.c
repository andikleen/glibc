#include "pthreadP.h"
#include "init-arch.h"
extern typeof(__pthread_rwlock_wrlock) __pthread_rwlock_wrlock_rtm;
extern typeof(__pthread_rwlock_wrlock) __pthread_rwlock_wrlock_nortm;
libm_ifunc(__pthread_rwlock_wrlock,
	   (HAS_RTM
	    ? __pthread_rwlock_wrlock_rtm
	    : __pthread_rwlock_wrlock_nortm))
strong_alias (__pthread_rwlock_wrlock, pthread_rwlock_wrlock)
libc_ifunc_hidden_def (__pthread_rwlock_wrlock)
