/* elision-lock.c: Elided pthread mutex lock.
   Copyright (C) 2011, 2012 Free Software Foundation, Inc.
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
#include "pthreadP.h"
#include "lowlevellock.h"
#include "hle.h"
#include "elision-conf.h"

#if !defined(LLL_LOCK) && !defined(EXTRAARG)
/* Make sure the configuration code is always linked in for static
   libraries. */
#include "elision-conf.c"
#endif

#ifndef EXTRAARG
#define EXTRAARG
#endif
#ifndef LLL_LOCK
#define LLL_LOCK(a,b) lll_lock(a,b), 0
#endif

#define aconf __elision_aconf

/* Adaptive lock using transactions. 
   By default the lock region is run as a transaction, and when it 
   aborts or the lock is busy the lock adapts itself.

   Requires a gcc with "asm goto" support (4.6+ or RedHat's 4.5). */

int 
__lll_lock_elision (int *futex, int *try_lock, EXTRAARG int private)
{
  if (*try_lock <= 0) 
    {	
      unsigned status;
      int try_xbegin;

      for (try_xbegin = aconf.retry_try_xbegin; 
	   try_xbegin > 0; 
	   try_xbegin--) 
	{ 
	  XBEGIN (fail);
	  if (*futex == 0)
	    return 0;
	
	  /* Lock was busy. Fall back to normal locking.  */
	  XEND ();

	  if (*try_lock != aconf.retry_lock_busy)
	    *try_lock = aconf.retry_lock_busy;
	  break;
		
	  /* transaction failure comes here */
	  XFAIL_STATUS (fail, status);

          if (__tsx_abort_hook)
              __tsx_abort_hook(status);

	  if (!(status & XABORT_RETRY))
	    { 
	      /* Internal abort. There is no chance for retry.
		 Use the normal locking and next time use lock.
		 Be careful to avoid writing to the lock. */
	      if (*try_lock != aconf.retry_lock_internal_abort)
		*try_lock = aconf.retry_lock_internal_abort;
	      break;
	    }
	}
    } 
  else 
    {
      /* Use a normal lock until the threshold counter runs out.	 
	 Lost updates possible. */
      (*try_lock)--;
    }

  /* Use a normal lock here */
  return LLL_LOCK ((*futex), private);
}
