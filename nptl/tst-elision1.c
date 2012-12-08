/* tst-elision1: Test basic elision success.
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

#if defined(__i386__) || defined(__x86_64__)
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

int disabled;

#include "sysdeps/unix/sysv/linux/x86/hle.h"

int
check (char *name, char *lock, int try, int txn)
{
  if (disabled) 
    {
      if (txn != 0)
	{
	  printf ("%s %s transaction run with PTHREAD_MUTEX=none\n", name, lock);
	  return 1;
	}
    }
  else
    {
      if (try == MAXTRY) 
	{
	  printf ("%s %s no transactions\n", name, lock);
	  return 1;
	}
    }
  return 0;
}

#define TESTLOCK(l, lock, unlock) 	\
  do					\
    {					\
      txn = 0;				\
      for (i = 0; i < ITER; i++)	\
	{				\
	  lock (&l);			\
	  if (XTEST ())			\
	    txn++;			\
	  unlock (&l); 			\
	}				\
    }						\
  while (try++ < MAXTRY && txn != expected);	\
  if (check (name, #lock, try, txn))		\
    return 1;

#include "tst-elision-common.c"

int
do_test (void)
{
  if (!cpu_has_rtm ())
    {
      printf ("elision test requires RTM capable CPU. not tested\n");
      return 0;
    }

  char *s = getenv("PTHREAD_MUTEX");
  if (s && !strcmp (s, "none"))
    disabled = 1;

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

