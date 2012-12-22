/* Automatically enable elision for existing user lock kinds */
#define FORCE_ELISION(m, s)						\
  if (__pthread_force_elision						\
      && (m->__data.__kind & PTHREAD_MUTEX_ELISION_FLAGS_NP) == 0	\
      && __is_smp)							\
    {									\
      mutex->__data.__kind |= PTHREAD_MUTEX_ELISION_NP;			\
      s;								\
    }
