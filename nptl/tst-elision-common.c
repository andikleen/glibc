/* tst-elision-common: Elision test harness.
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
   <http://www.gnu.org/licenses/>.  */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "config.h"

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

#ifndef USE_TRYLOCK_ONLY
static int
pthread_mutex_timedlock_wrapper(pthread_mutex_t *l)
{
  struct timespec wait = { 0, 0 };
  return pthread_mutex_timedlock (l, &wait);
}
#endif

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

#ifndef USE_TRYLOCK_ONLY
  TESTLOCK(lock, pthread_mutex_lock, pthread_mutex_unlock, force);
  TESTLOCK(lock, pthread_mutex_timedlock_wrapper, pthread_mutex_unlock, force);
  TESTLOCK(lock, pthread_mutex_trylock, pthread_mutex_unlock, force);
#else
  TESTTRYLOCK(lock, pthread_mutex_lock, pthread_mutex_trylock, pthread_mutex_unlock, force);
#endif

  err = pthread_mutex_destroy (&lock);
  if (err != 0)
    {
      printf ("destroy for %s failed: %d\n", name, err);
      return 1;
    }
  return 0;
}

static int
run_mutex_init (int iter, const char *name, int type, int has_type, int force,
		int elision)
{
  pthread_mutexattr_t attr;

  pthread_mutexattr_init (&attr);
  if (type)
    pthread_mutexattr_settype (&attr, type);
  if (elision >= 0)
    pthread_mutexattr_setelision_np (&attr, elision);

  pthread_mutex_init (&lock, has_type ? &attr : NULL);
  return run_mutex (iter, name, force);
}

static int
default_elision_enabled (void)
{
#ifdef ENABLE_LOCK_ELISION
  return 1;
#else
  return 0;
#endif
}

int
mutex_test (void)
{
  int ret = 0;
  int default_iter = default_elision_enabled () ? ITER : 0;

  lock = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;
  ret += run_mutex (default_iter, "default initializer timed", 0);

  ret += run_mutex_init (default_iter, "timed init default", 0, 0, 0, -1);
  ret += run_mutex_init (ITER, "timed init elision",
                         PTHREAD_MUTEX_TIMED_NP, 1, 1, 1);
  ret += run_mutex_init (ITER, "timed init no elision",
                         PTHREAD_MUTEX_TIMED_NP, 1, 2, 0);
  ret += run_mutex_init (0, "normal no elision",
			 PTHREAD_MUTEX_NORMAL, 1, 2, -1);

  return ret;
}

pthread_rwlock_t rwlock;

#ifndef USE_TRYLOCK_ONLY
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
#endif

int
run_rwlock (int expected, const char *name, int force)
{
  int i;
  int try = 0;
  int txn __attribute__((unused));
  int err;

#ifndef USE_TRYLOCK_ONLY
  TESTLOCK(rwlock, pthread_rwlock_rdlock, pthread_rwlock_unlock, force);
  TESTLOCK(rwlock, pthread_rwlock_wrlock, pthread_rwlock_unlock, force);
  TESTLOCK(rwlock, pthread_rwlock_rdlock, pthread_rwlock_unlock, force);
  TESTLOCK(rwlock, pthread_rwlock_tryrdlock, pthread_rwlock_unlock, force);
  TESTLOCK(rwlock, pthread_rwlock_trywrlock, pthread_rwlock_unlock, force);
  TESTLOCK(rwlock, pthread_rwlock_timedrdlock_wrapper,
	   pthread_rwlock_unlock, force);
  TESTLOCK(rwlock, pthread_rwlock_timedwrlock_wrapper,
	   pthread_rwlock_unlock, force);
#else
  TESTTRYLOCK(rwlock, pthread_rwlock_wrlock, pthread_rwlock_trywrlock,
	      pthread_rwlock_unlock, force);
#endif

  err = pthread_rwlock_destroy (&rwlock);
  if (err != 0)
    {
      printf ("pthread_rwlock_destroy for %s failed: %d\n", name, err);
      return 1;
    }
  return 0;
}

int
run_rwlock_attr (int iter, const char *name, int type, int el, int force)
{
  pthread_rwlockattr_t attr;
  pthread_rwlockattr_init (&attr);
  pthread_rwlockattr_setkind_np (&attr, type);
  if (el >= 0)
    pthread_rwlockattr_setelision_np (&attr, el);
  pthread_rwlock_init (&rwlock, &attr);
  return run_rwlock (iter, name, force);
}

int
run_rwlock_attr_set (int iter, const char *extra, int flag, int force)
{
  char str[100];
  int ret = 0;

  snprintf(str, sizeof str, "rwlock attr prefer reader %s", extra);
  ret += run_rwlock_attr (iter, str,
                          PTHREAD_RWLOCK_PREFER_READER_NP, flag, force);
  snprintf(str, sizeof str, "rwlock attr prefer writer %s", extra);
  ret += run_rwlock_attr (iter, str,
                          PTHREAD_RWLOCK_PREFER_WRITER_NP, flag, force);
  snprintf(str, sizeof str, "rwlock attr prefer writer non recursive %s", extra);
  ret += run_rwlock_attr (iter, str,
		          PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP, flag, force);
  return ret;
}


int
rwlock_test (void)
{
  int ret = 0;
  int default_iter = default_elision_enabled () ? ITER : 0;

  pthread_rwlock_init (&rwlock, NULL);
  ret += run_rwlock (default_iter, "rwlock created", 0);

  rwlock = (pthread_rwlock_t)PTHREAD_RWLOCK_INITIALIZER;
  ret += run_rwlock (default_iter, "rwlock initialized", 0);

  rwlock = (pthread_rwlock_t)PTHREAD_RWLOCK_WRITER_NONRECURSIVE_INITIALIZER_NP;
  ret += run_rwlock (default_iter, "rwlock initialized writer non recursive", 0);

  rwlock = (pthread_rwlock_t)PTHREAD_RWLOCK_WRITER_NONRECURSIVE_INITIALIZER_NP;
  ret += run_rwlock (default_iter, "rwlock initialized writer non recursive", 0);

  ret += run_rwlock_attr_set (default_iter, "", 0, 0);

  ret += run_rwlock_attr_set (ITER, "eliding", 1, 1);
  ret += run_rwlock_attr_set (0, "not eliding", 0, 2);

  return ret;
}
