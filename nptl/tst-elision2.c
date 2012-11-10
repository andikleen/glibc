/* tst-elision2: Test TSX abort hook.
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

#if defined(__i386__) || defined(__x86_64__)
#include <pthread.h>
#include <stdio.h>
#include <hle.h>
#include <cpuid.h>

FILE *null;
int abort_count;

void do_abort(unsigned code)
{
  abort_count++;

  /* Do something here that clobbers registers */
  fprintf (null, "abort %x %d\n", code, abort_count);
}

#define TESTLOCK(l, lock, unlock, force)	\
   do					\
    {					\
      abort_count = 0;			\
      fprintf (null, "start %s %s\n", #lock, name); \ 
      for (i = 0; i < ITER; i++)	\
	{				\
	  lock (&l);			\
	  _xabort(1);			\
	  unlock (&l);			\
	}				\
    }					\
  while (try++ < MAXTRY && abort_count != ITER);\
  if (abort_count != ITER && force != 2)	\
    {						\
      printf ("%s %s abort hook did not work %d\n", name, #lock, abort_count); \
      return 1;					\
    }

#include "tst-elision-common.c"

int
do_test (void)
{
  if (!cpu_has_rtm ())
    {
      printf ("elision test requires RTM capable CPU. not tested\n");
      return 0;
    }

  null = fopen ("/dev/null", "w");
  if (!null)
    null = stdout;

  __pthread_set_elision_abort_hook (do_abort);

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
