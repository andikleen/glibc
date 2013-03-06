/* pthread_rwlock_tryrdlock: Lock eliding version of pthreads rwlock_tryrdlock.
   Copyright (C) 2013 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>. */
#include <pthread.h>
#include <pthreadP.h>
#include <hle.h>
#include <elision-conf.h>
#include "init-arch.h"

#define __pthread_rwlock_tryrdlock __full_pthread_rwlock_tryrdlock
#include <nptl/pthread_rwlock_tryrdlock.c>
#undef __pthread_rwlock_tryrdlock

int
__pthread_rwlock_tryrdlock(pthread_rwlock_t *rwlock)
{
  unsigned status;

  if ((rwlock->__data.__eliding > 0 && __elision_available)
      || (rwlock->__data.__eliding == 0 && __rwlock_rtm_enabled))
    {
      if ((status = _xbegin()) == _XBEGIN_STARTED)
	{
	  if (rwlock->__data.__writer == 0
	      && rwlock->__data.__nr_readers == 0)
	    return 0;
	  /* Lock was busy. Fall back to normal locking.
	     Could also _xend here but xabort with 0xff code
	     is more visible in the profiler. */
	  _xabort (0xff);
	}
      /* Aborts come here */
    }

  return __full_pthread_rwlock_tryrdlock (rwlock);
}

strong_alias(__pthread_rwlock_tryrdlock, pthread_rwlock_tryrdlock);
