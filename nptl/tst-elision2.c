/* tst-elision2: Test elided trylock semantics
   Copyright (C) 2014 Free Software Foundation, Inc.
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

/* To use on other architectures you would need an own version of
   cpu_has_rtm.  */
#if defined(__i386__) || defined(__x86_64__)
# include <pthread.h>
# include <stdio.h>
# include <hle.h>
# include <cpuid.h>

int check (int success, int force)
{
  /* Any nested trylock disabled right now. */
  return success == 0;
}

# define TESTTRYLOCK(l, lock, trylock, unlock, force)	\
   do					\
    {					\
      txn = 0;				\
      for (i = 0; i < ITER; i++)	\
	{				\
	  lock (&l);			\
	  if (trylock (&l) == 0)	\
	    {				\
	      txn++;			\
	      unlock (&l);		\
	    }				\
	  unlock (&l);			\
	}				\
    }					\
   while (try++ < MAXTRY && txn != ITER);				\
   if (!check (txn == ITER, force))					\
     {									\
       printf ("%s %s nested trylock check failed txn:%d iter:%d force:%d\n", \
					name, #lock, txn, ITER, force);	\
       return 1;							\
     }

# define USE_TRYLOCK_ONLY 1
# include "tst-elision-common.c"

int
do_test (void)
{
  if (cpu_has_rtm () == 0)
    {
      printf ("elision test requires RTM capable CPU. not tested\n");
      return 0;
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
