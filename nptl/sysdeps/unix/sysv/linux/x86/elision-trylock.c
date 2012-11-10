/* elision-trylock.c: Lock eliding trylock for pthreads.
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
#include <lowlevellock.h>
#include "hle.h"
#include "elision-conf.h"

#define aconf __elision_aconf

/* Try to elide a futex trylock. FUTEX is the futex variable. TRY_LOCK is the
   adaptation counter in the mutex. UPGRADED is != 0 when this is for an
   automatically upgraded lock.  */

int
__lll_trylock_elision (int *futex, short *try_lock, int upgraded)
{
  /* Only try a transaction if it's worth it */
  if (*try_lock <= 0)
    {
      unsigned status;

      /* When this could be a nested trylock that is not explicitely
	 declared an elided lock abort. This makes us follow POSIX
	 paper semantics. */
      if (upgraded)
        _xabort (_ABORT_NESTED_TRYLOCK);

      if ((status = _xbegin()) == _XBEGIN_STARTED)
	{
	  if (*futex == 0)
	    return 0;

	  /* Lock was busy. Fall back to normal locking.
	     Could also _xend here but xabort with 0xff code
	     is more visible in the profiler. */
	  _xabort (_ABORT_LOCK_BUSY);
	}

      if (!(status & _XABORT_RETRY))
        {
          /* Internal abort. No chance for retry. For future
             locks don't try speculation for some time. */
          if (*try_lock != aconf.retry_trylock_internal_abort)
            *try_lock = aconf.retry_trylock_internal_abort;
        }
    }
  else
    {
      /* Lost updates are possible, but harmless. */
      (*try_lock)--;
    }

  return lll_trylock (*futex);
}
