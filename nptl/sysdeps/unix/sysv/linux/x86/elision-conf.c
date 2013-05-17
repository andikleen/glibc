/* elision-conf.c: Lock elision tunable parameters.
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

#include "config.h"
#include <pthreadP.h>
#include <sys/fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <init-arch.h>
#include <elision-conf.h>
#include <glibc-var.h>

struct elision_config __elision_aconf =
  {
    .skip_lock_busy = 3,
    .skip_lock_internal_abort = 3,
    .retry_try_xbegin = 3,
    .skip_trylock_internal_abort = 3,
  };

struct tune
{
  const char *name;
  unsigned offset;
  int len;
};

#define FIELD(x) { #x, offsetof(struct elision_config, x), sizeof(#x)-1 }

static const struct tune tunings[] =
  {
    FIELD(skip_lock_busy),
    FIELD(skip_lock_internal_abort),
    FIELD(retry_try_xbegin),
    FIELD(skip_trylock_internal_abort),
    {}
  };

#define PAIR(x) x, sizeof (x)-1

/* Complain.  */

static void
complain (const char *msg, int len)
{
  INTERNAL_SYSCALL_DECL (err);
  INTERNAL_SYSCALL (write, err, 3, 2, (char *)msg, len);
}

/* Parse configuration information.  */

static void
elision_aconf_setup (const char *s)
{
  int i;

  while (*s)
    {
      for (i = 0; tunings[i].name != NULL; i++)
	{
	  int nlen = tunings[i].len;

	  if (strncmp (tunings[i].name, s, nlen) == 0)
	    {
	      char *end;
	      int val;

	      if (s[nlen] != '=')
		{
  		  complain (PAIR ("pthreads: invalid GLIBC_PTHREAD_MUTEX syntax: missing =\n"));
	 	  return;
		}
	      s += nlen + 1;
	      val = strtoul (s, &end, 0);
	      if (end == s)
		{
  		  complain (PAIR ("pthreads: invalid GLIBC_PTHREAD_MUTEX syntax: missing number\n"));
	 	  return;
		}
	      *(int *)(((char *)&__elision_aconf) + tunings[i].offset) = val;
	      s = end;
	      if (*s == ',' || *s == ':')
		s++;
	      else if (*s)
		{
  		  complain (PAIR ("pthreads: invalid GLIBC_PTHREAD_MUTEX syntax: garbage after number\n"));
	 	  return;
		}
	      break;
	    }
	}
      if (tunings[i].name == NULL)
      	{
  	  complain (PAIR ("pthreads: invalid GLIBC_PTHREAD_MUTEX syntax: unknown tunable\n"));
 	  return;
	}
    }
}

/* Elided rwlock toggle.  */

int __rwlock_rtm_enabled attribute_hidden;

/* Retries for elided rwlocks.  */

int __rwlock_rtm_read_retries attribute_hidden = 3;

/* Global elision check switch.  */

int __elision_available attribute_hidden;

/* Initialize elision mutex.  */

static void
elision_mutex_init (const char *s)
{
  if (s == NULL)
    {
#ifdef ENABLE_LOCK_ELISION
      __pthread_force_elision = __elision_available;
#endif
      return;
    }

  if (strncmp (s, "adaptive", 8) == 0 && (s[8] == 0 || s[8] == ':'))
    {
      __pthread_force_elision = __elision_available;
      if (s[8] == ':')
	elision_aconf_setup (s + 9);
    }
  else if (strncmp (s, "elision", 7) == 0 && (s[7] == 0 || s[7] == ':'))
    {
      __pthread_force_elision = __elision_available;
      if (s[7] == ':')
        elision_aconf_setup (s + 8);
    }
  else if (strncmp (s, "none", 4) == 0 && s[4] == 0)
    __pthread_force_elision = 0;
  else
    complain (PAIR ("pthreads: Unknown setting for GLIBC_PTHREAD_MUTEX\n"));
}

/* Initialize elision rwlock.  */

static void
elision_rwlock_init (const char *s)
{
  if (s == NULL)
    {
#ifdef ENABLE_LOCK_ELISION
      __rwlock_rtm_enabled = __elision_available;
#endif
      return;
    }
  if (strncmp (s, "elision", 7) == 0)
    {
      __rwlock_rtm_enabled = __elision_available;
      if (s[7] == ':')
        {
          char *end;
	  int n;

          n = strtoul (s + 8, &end, 0);
	  if (end == s + 8)
	    complain (PAIR ("pthreads: Bad retry number for GLIBC_PTHREAD_RWLOCK\n"));
          else
	    __rwlock_rtm_read_retries = n;
	}
    }
  else if (strcmp (s, "none") == 0)
    __rwlock_rtm_enabled = 0;
  else
    complain (PAIR ("pthreads: Unknown setting for GLIBC_PTHREAD_RWLOCK\n"));
}

/* Initialize elison.  */

static void
elision_init (int argc __attribute__ ((unused)),
	      char **argv  __attribute__ ((unused)),
	      char **environ)
{
  __elision_available = HAS_RTM;

  /* For static builds need to call this explicitely. Noop for dynamic.  */
  __glibc_var_init (argc, argv, environ);
  elision_mutex_init (_dl_glibc_var[GLIBC_VAR_PTHREAD_MUTEX].val);
  elision_rwlock_init (_dl_glibc_var[GLIBC_VAR_PTHREAD_RWLOCK].val);
}

#ifdef SHARED
# define INIT_SECTION ".init_array"
#else
# define INIT_SECTION ".preinit_array"
#endif

void (*const __pthread_init_array []) (int, char **, char **)
  __attribute__ ((section (INIT_SECTION), aligned (sizeof (void *)))) =
{
  &elision_init
};
