/* elision-conf.c: Lock elision tunable parameters.
   Copyright (C) 2013-2014 Free Software Foundation, Inc.
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

#include "config.h"
#include <pthreadP.h>
#include <init-arch.h>
#include <elision-conf.h>
#include <unistd.h>
#include <glibc-var.h>

/* Reasonable initial tuning values, may be revised in the future.
   This is a conservative initial value.  */

struct elision_config __elision_aconf =
  {
    /* How often to not attempt to use elision if a transaction aborted
       because the lock is already acquired.  Expressed in number of lock
       acquisition attempts.  */
    .skip_lock_busy = 3,
    /* How often to not attempt to use elision if a transaction aborted due
       to reasons other than other threads' memory accesses.  Expressed in
       number of lock acquisition attempts.  */
    .skip_lock_internal_abort = 3,
    /* How often we retry using elision if there is chance for the transaction
       to finish execution (e.g., it wasn't aborted due to the lock being
       already acquired.  */
    .retry_try_xbegin = 3,
    /* Same as SKIP_LOCK_INTERNAL_ABORT but for trylock.  */
    .skip_trylock_internal_abort = 3,
  };

struct elision_config __elision_rwconf =
  {
    /* How often to not attempt to use elision if a transaction aborted
       because the lock is already acquired.  Expressed in number of lock
       acquisition attempts.  */
    .skip_lock_busy = 3,
    /* How often to not attempt to use elision if a transaction aborted due
       to reasons other than other threads' memory accesses.  Expressed in
       number of lock acquisition attempts.  */
    .skip_lock_internal_abort = 3,
    /* How often we retry using elision if there is chance for the transaction
       to finish execution (e.g., it wasn't aborted due to the lock being
       already acquired.  */
    .retry_try_xbegin = 3,
    /* Same as SKIP_LOCK_INTERNAL_ABORT but for trylock.  */
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

/* Parse configuration information for string S into CONFIG.  */

static void
elision_conf_setup (const char *s, struct elision_config *config)
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
		  complain (PAIR ("pthreads: invalid GLIBC_PTHREAD_* syntax: missing =\n"));
		  return;
		}
	      s += nlen + 1;
	      val = strtoul (s, &end, 0);
	      if (end == s)
		{
		  complain (PAIR ("pthreads: invalid GLIBC_PTHREAD_* syntax: missing number\n"));
		  return;
		}
	      *(int *)(((char *)config) + tunings[i].offset) = val;
	      s = end;
	      if (*s == ',' || *s == ':')
		s++;
	      else if (*s)
		{
		  complain (PAIR ("pthreads: invalid GLIBC_PTHREAD_* syntax: garbage after number\n"));
		  return;
		}
	      break;
	    }
	}
      if (tunings[i].name == NULL)
	{
	  complain (PAIR ("pthreads: invalid GLIBC_PTHREAD_* syntax: unknown tunable\n"));
	  return;
	}
    }
}

/* Set when the CPU supports elision.  When false elision is never attempted.
 */

int __elision_available attribute_hidden;

/* Force elision for all new locks.  This is used to decide whether existing
   DEFAULT locks should be automatically upgraded to elision in
   pthread_mutex_lock().  Disabled for suid programs.  Only used when elision
   is available.  */

int __pthread_force_elision attribute_hidden;

/* Initialize mutex elision.  */

static void
elision_mutex_init (const char *s)
{
  if (s == NULL)
    return;

  if (strncmp (s, "elision", 7) == 0 && (s[7] == 0 || s[7] == ':'))
    {
      __pthread_force_elision = __elision_available;
      if (s[7] == ':')
	elision_conf_setup (s + 8, &__elision_aconf);
    }
  else if (strcmp (s, "none") == 0 || strcmp (s, "no_elision") == 0)
    __pthread_force_elision = 0;
  else
    complain (PAIR ("pthreads: Unknown setting for GLIBC_PTHREAD_MUTEX\n"));
}

/* Initialize elision for rwlocks.  */

static void
elision_rwlock_init (const char *s)
{
  if (s == NULL)
    return;
  if (strncmp (s, "elision", 7) == 0)
    {
      __elision_rwconf.retry_try_xbegin = __elision_available;
      if (s[7] == ':')
	elision_conf_setup (s + 8, &__elision_rwconf);
    }
  else if (strcmp (s, "none") == 0 || strcmp (s, "no_elision") == 0)
    __elision_rwconf.retry_try_xbegin = 0; /* Disable elision on rwlocks */
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
#ifdef ENABLE_LOCK_ELISION
  __pthread_force_elision = __libc_enable_secure ? 0 : __elision_available;
#endif
  if (!HAS_RTM)
    __elision_rwconf.retry_try_xbegin = 0; /* Disable elision on rwlocks */


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
