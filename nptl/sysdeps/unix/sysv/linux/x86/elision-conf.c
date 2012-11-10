/* elision-conf.c: Lock elision tunable parameters.
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
   <http://www.gnu.org/licenses/>. */
#include <pthreadP.h>
#include <sys/fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <init-arch.h>
#include "elision-conf.h"

struct cpu_features __cpu_features attribute_hidden;

struct elision_config __elision_aconf = 
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

#define FIELD(x) { #x, offsetof(struct elision_config, x) }

static const struct tune tunings[] = 
  {
    FIELD(retry_lock_busy),
    FIELD(retry_lock_internal_abort),
    FIELD(retry_try_xbegin),
    FIELD(retry_trylock_internal_abort),
    {}
  };

#define PAIR(x) x, sizeof (x)-1

void 
elision_aconf_setup(const char *s)
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
	      *(int *)(((char *)&__elision_aconf) + tunings[i].offset) = val;
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


int __rwlock_rtm_enabled attribute_hidden;
int __rwlock_rtm_read_retries attribute_hidden = 3;
int __elision_available attribute_hidden;

#define PAIR(x) x, sizeof (x)-1

static void __attribute__((constructor)) 
elision_init (void)
{
  char *s;

  if (cpu_has_rtm()) 
    {
      __pthread_force_elision = 1;
      __elision_available = 1;
    }
  s = getenv ("PTHREAD_MUTEX");
  if (!s)
    goto check_rwlock;
  if (!strncmp (s, "adaptive", 8) && (s[8] == 0 || s[8] == ':'))
    {
      __pthread_force_elision = __elision_available;
      if (s[8] == ':')
	elision_aconf_setup (s + 9);
    }
  else if (!strncmp (s, "elision", 7) && (s[7] == 0 || s[7] == ':'))
    {
      __pthread_force_elision = __elision_available;
      if (s[7] == ':')
        elision_aconf_setup (s + 8);
    }	    
  else if (!strcmp (s, "none"))
    __pthread_force_elision = 0;
  else 
    __write (2, PAIR("pthreads: Unknown setting for PTHREAD_MUTEX\n"));

check_rwlock:
  s = getenv ("PTHREAD_RWLOCK");
  if (!s)
    {
      __rwlock_rtm_enabled = __elision_available;
      return;
    }
  if (!strncmp (s, "elision", 7))
    {
      __rwlock_rtm_enabled = __elision_available;
      if (s[7] == ':')
        {
          char *end;
	  int n;

          n = strtoul (s + 8, &end, 0);
	  if (end == s + 8)
    	    __write (2, PAIR("pthreads: Bad retry number for PTHREAD_RWLOCK\n"));
          else
	    __rwlock_rtm_read_retries = n;
	}
    }
  else if (!strcmp(s, "none"))
    __rwlock_rtm_enabled = 0;
  else
    __write (2, PAIR("pthreads: Unknown setting for PTHREAD_RWLOCK\n"));
}

/* Allow user programs to hook into the abort handler.
   This is useful for debugging situations where you need to get 
   information out of a transaction. */

__pthread_abort_hook_t __tsx_abort_hook attribute_hidden;

__pthread_abort_hook_t __pthread_set_abort_hook(__pthread_abort_hook_t hook)
{
  __pthread_abort_hook_t old = __tsx_abort_hook;
  __tsx_abort_hook = hook;
  return old;
}

