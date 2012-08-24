/* Copyright (C) 2012 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Andi Kleen

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "adaptive-conf.h"

struct hle_adaptive_config __hle_aconf = 
  { 
    .retry_lock_busy = 3, 
    .retry_lock_internal_abort = 3,
    .retry_try_xbegin = 3,
    .retry_trylock_internal_abort = 3,
  };

struct tune 
{ 
  const char *name;
  unsigned offset;
};

#define FIELD(x) { #x, offsetof(struct hle_adaptive_config, x) }

static const struct tune tunings[] = 
  {
    FIELD(retry_lock_busy),
    FIELD(retry_lock_internal_abort),
    FIELD(retry_try_xbegin),
    FIELD(retry_trylock_internal_abort),
    {}
  };

#define PAIR(x) x, sizeof (x)-1

void hle_aconf_setup(const char *s)
{
  int i;

  while (*s) 
    {
      for (i = 0; tunings[i].name; i++)
	{
	  int nlen = strlen (tunings[i].name);
	  
	  if (!strncmp (tunings[i].name, s, nlen) && s[nlen] == ':')
	    {
	      char *end;
	      int val;
	      
	      s += nlen + 1;
	      val = strtoul (s, &end, 0);
	      if (end == s)
		goto error;
	      *(int *)(((char *)&__hle_aconf) + tunings[i].offset) = val;
	      s = end;
	      if (*s == ',')
		s++;
	      else if (*s)
		goto error;
	    }
	}
    }
  return;
      
 error:
  __write (2, PAIR("pthreads: invalid PTHREAD_MUTEX syntax\n"));
}

/* Allow user programs to hook into the abort handler.
   This is useful for debugging situations where you need to get 
   information out of a transaction. */

tsx_abort_hook_t __tsx_abort_hook;

tsx_abort_hook_t __set_abort_hook(tsx_abort_hook_t hook)
{
  tsx_abort_hook_t old = __tsx_abort_hook;
  __tsx_abort_hook = hook;
  return old;
}

