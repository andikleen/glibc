/* tst-elision1: Test basic elision success.
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

/* To use on other architectures you would need an own version
   of cpu_has_rtm and _xtest.  */
#if defined(__i386__) || defined(__x86_64__)
# include <elision-conf.h>
# include <pthread.h>
# include <stdlib.h>
# include <string.h>
# include <stdio.h>
# include <hle.h>

int disabled;
int forced;

int
check (const char *name, const char *lock, int try, int txn, int max,
       int override)
{
  int should_run = 1;

  if (override == 0)
    should_run = disabled == 0;
  else if (override == 1)
    should_run = 1;
  else if (override == 2)
    should_run = 0;

  /* forced noop right now, so not tested. But test if the defaults change.  */
  if (!should_run)
    {
      if (txn != 0)
	{
	  printf ("%s %s transaction run unexpected\n", name, lock);
	  return 1;
	}
    }
  else
    {
      if (try == max)
	{
	  printf ("%s %s no transactions when expected\n", name, lock);
	  return 1;
	}
    }
  return 0;
}

# define TESTLOCK(l, lock, unlock, force)\
  do					\
    {					\
      txn = 0;				\
      for (i = 0; i < ITER; i++)	\
	{				\
	  lock (&l);			\
	  if (_xtest ())		\
	    txn++;			\
	  unlock (&l);			\
	}				\
    }						\
  while (try++ < MAXTRY && txn != expected);	\
  if (check (name, #lock, try, txn, MAXTRY, force))	\
    return 1;

# include "tst-elision-common.c"

int
do_test (void)
{
  if (cpu_has_rtm () == 0)
    {
      printf ("elision test requires RTM capable CPU. not tested\n");
      return 0;
    }

  char *s = getenv ("GLIBC_PTHREAD_MUTEX");
  if (s != NULL)
    {
      char *o = getenv ("GLIBC_PTHREAD_RWLOCK");
      if (o == NULL || strcmp (o, s))
        {
          puts("GLIBC_PTHREAD_MUTEX and GLIBC_PTHREAD_RWLOCK must match for test!\n");
          return 1;
	}
      if (strcmp (s, "none") == 0)
        disabled = 1;
      if (strcmp (s, "elision") == 0)
	forced = 1;
    }

  if (mutex_test ())
    return 1;

  if (rwlock_test ())
    return 1;

  return 0;
}
#else
int do_test (void)
{
  return 0;
}
#endif

#define TEST_FUNCTION do_test ()
#include "../test-skeleton.c"
