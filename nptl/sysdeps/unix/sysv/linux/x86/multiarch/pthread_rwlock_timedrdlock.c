#include "pthreadP.h"
#include "init-arch.h"
extern typeof(pthread_rwlock_timedrdlock) __pthread_rwlock_timedrdlock_rtm;
extern typeof(pthread_rwlock_timedrdlock) __pthread_rwlock_timedrdlock_nortm;
libm_ifunc(pthread_rwlock_timedrdlock,
	   (HAS_RTM
	    ? __pthread_rwlock_timedrdlock_rtm
	    : __pthread_rwlock_timedrdlock_nortm))
