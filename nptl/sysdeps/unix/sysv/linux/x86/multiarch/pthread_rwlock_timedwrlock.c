#include "pthreadP.h"
#include "init-arch.h"
extern typeof(pthread_rwlock_timedwrlock) __pthread_rwlock_timedwrlock_rtm;
extern typeof(pthread_rwlock_timedwrlock) __pthread_rwlock_timedwrlock_nortm;
libm_ifunc(pthread_rwlock_timedwrlock,
	   (HAS_RTM
	    ? __pthread_rwlock_timedwrlock_rtm
	    : __pthread_rwlock_timedwrlock_nortm))
