/* tst-elision-common: Elision test harness.
   Copyright (C) 2012 Free Software Foundation, Inc.
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
   <http://www.gnu.org/licenses/>.  */

#include <stdio.h>
#include <cpuid.h>

#define CPUID_FEATURE_RTM (1U << 11)

static int 
cpu_has_rtm (void)
{
  if (__get_cpuid_max (0, NULL) >= 7)
    {
      unsigned a, b, c, d;

      __cpuid_count (7, 0, a, b, c, d);
      if (b & CPUID_FEATURE_RTM) 
	return 1;
    }
  return 0;
}

#define ITER 10
#define MAXTRY 100

pthread_mutex_t lock;

static int
pthread_mutex_timedlock_wrapper(pthread_mutex_t *l)
{
  struct timespec wait = { 0, 0 };
  return pthread_mutex_timedlock (l, &wait);
}

/* Note this test program will fail when single stepped.
   It also assumes that simple transactions always work. There is no
   guarantee in the architecture that this is the case. We do some 
   retries to handle random abort cases like interrupts. But it's
   not fully guaranteed. However when this fails it is somewhat worrying. */

int
run_mutex (int expected, const char *name, int force)
{
  int i;
  int try = 0;
  int txn __attribute__((unused));
  int err;

  TESTLOCK(lock, pthread_mutex_lock, pthread_mutex_unlock, force);
  TESTLOCK(lock, pthread_mutex_trylock, pthread_mutex_unlock, force);
  TESTLOCK(lock, pthread_mutex_timedlock_wrapper, pthread_mutex_unlock, force);

  err = pthread_mutex_destroy (&lock);
  if (err != 0) 
    {
      printf ("destroy for %s failed: %d\n", name, err);
      return 1;
    }
  return 0;
}

static int
run_mutex_init (int iter, const char *name, int type, int has_type, int force)
{
  pthread_mutexattr_t attr;

  pthread_mutexattr_init (&attr);
  pthread_mutexattr_settype (&attr, type);
  pthread_mutex_init (&lock, has_type ? &attr : NULL);
  return run_mutex (iter, name, force);
}

/* This assumes elision is enabled by default. If that changes change
   the first arguments of the default cases to 0. */

int
mutex_test (void)
{
  int ret = 0;

  lock = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;
  ret += run_mutex (ITER, "default initializer timed", 0);
  lock = (pthread_mutex_t) PTHREAD_TIMED_ELISION_MUTEX_INITIALIZER_NP;
  ret += run_mutex (ITER, "timed initializer elision", 1);
  lock = (pthread_mutex_t) PTHREAD_TIMED_NO_ELISION_MUTEX_INITIALIZER_NP;
  ret += run_mutex (0, "timed initializer no elision", 2);

  lock = (pthread_mutex_t) PTHREAD_ADAPTIVE_MUTEX_INITIALIZER_NP;
  run_mutex (ITER, "adaptive initializer default", 0);
  lock = (pthread_mutex_t) PTHREAD_ADAPTIVE_ELISION_MUTEX_INITIALIZER_NP;
  ret += run_mutex (ITER, "adaptive initializer elision", 1);
  lock = (pthread_mutex_t) PTHREAD_ADAPTIVE_NO_ELISION_MUTEX_INITIALIZER_NP;
  ret += run_mutex (0, "adaptive initializer no elision", 2);
  
  ret += run_mutex_init (ITER, "timed init default", 0, 0, 0);
  ret += run_mutex_init (ITER, "timed init elision",
                         PTHREAD_MUTEX_TIMED_ELISION_NP, 1, 1);
  ret += run_mutex_init (0, "timed init no elision",
			 PTHREAD_MUTEX_TIMED_NO_ELISION_NP, 1, 2);

  ret += run_mutex_init (ITER, "adaptive init default", 0, 0, 0);
  ret += run_mutex_init (ITER, "adaptive init elision", 
                         PTHREAD_MUTEX_ADAPTIVE_ELISION_NP, 1, 1);
  ret += run_mutex_init (0, "adaptive init no elision",
		         PTHREAD_MUTEX_ADAPTIVE_NO_ELISION_NP, 
			  1, 2);

  return ret;
}

pthread_rwlock_t rwlock;

static int
pthread_rwlock_timedwrlock_wrapper(pthread_rwlock_t *l)
{
  struct timespec wait = { 0, 0 };
  return pthread_rwlock_timedwrlock (l, &wait);
}

static int
pthread_rwlock_timedrdlock_wrapper(pthread_rwlock_t *l)
{
  struct timespec wait = { 0, 0 };
  return pthread_rwlock_timedrdlock (l, &wait);
}

int
run_rwlock (int expected, const char *name, int force)
{
  int i;
  int try = 0;
  int txn __attribute__((unused));
  int err;

  TESTLOCK(rwlock, pthread_rwlock_rdlock, pthread_rwlock_unlock, force);
  TESTLOCK(rwlock, pthread_rwlock_wrlock, pthread_rwlock_unlock, force);
  TESTLOCK(rwlock, pthread_rwlock_rdlock, pthread_rwlock_unlock, force);
  TESTLOCK(rwlock, pthread_rwlock_tryrdlock, pthread_rwlock_unlock, force);
  TESTLOCK(rwlock, pthread_rwlock_trywrlock, pthread_rwlock_unlock, force);
  TESTLOCK(rwlock, pthread_rwlock_timedrdlock_wrapper, 
	   pthread_rwlock_unlock, force);
  TESTLOCK(rwlock, pthread_rwlock_timedwrlock_wrapper, 
	   pthread_rwlock_unlock, force);

  err = pthread_rwlock_destroy (&rwlock);
  if (err != 0)
    {
      printf ("pthread_rwlock_destroy for %s failed: %d\n", name, err);
      return 1;
    }
  return 0;
}

int
run_rwlock_attr (int iter, const char *name, int type, int force)
{
  pthread_rwlockattr_t attr;
  pthread_rwlockattr_init (&attr);
  pthread_rwlockattr_setkind_np (&attr, type);
  pthread_rwlock_init (&rwlock, &attr);
  return run_rwlock (iter, name, force);
}

int
rwlock_test (void)
{
  int ret = 0;

  pthread_rwlock_init (&rwlock, NULL);
  ret += run_rwlock (ITER, "rwlock created", 0);

  rwlock = (pthread_rwlock_t)PTHREAD_RWLOCK_INITIALIZER;
  ret += run_rwlock (ITER, "rwlock initialized", 0);

  rwlock = (pthread_rwlock_t)PTHREAD_RWLOCK_WRITER_NONRECURSIVE_INITIALIZER_NP;
  ret += run_rwlock (ITER, "rwlock initialized writer non recursive", 0);

  rwlock = (pthread_rwlock_t)PTHREAD_RWLOCK_WRITER_NONRECURSIVE_INITIALIZER_NP;
  ret += run_rwlock (ITER, "rwlock initialized writer non recursive", 0);

#ifdef PTHREAD_RWLOCK_WRITER_INITIALIZER_NP
  // XXX includes are missing PTHREAD_RWLOCK_WRITER_INITIALIZER_NP
  rwlock = (pthread_rwlock_t)PTHREAD_RWLOCK_WRITER_INITIALIZER_NP;
  ret += run_rwlock (ITER, "rwlock initialized writer", 0);
#endif

  ret += run_rwlock_attr (ITER, "rwlock attr prefer reader",
                          PTHREAD_RWLOCK_PREFER_READER_NP, 0);
  ret += run_rwlock_attr (ITER, "rwlock attr prefer writer",
                          PTHREAD_RWLOCK_PREFER_WRITER_NP, 0);
  ret += run_rwlock_attr (ITER, "rwlock attr prefer writer non recursive",
		          PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP, 0);

  // XXX versions with elision disabled

  return ret; 
}
