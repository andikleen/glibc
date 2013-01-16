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
#include <pthreadP.h>
#include <sys/fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <init-arch.h>
#include "elision-conf.h"

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
  int len;
};

#define FIELD(x) { #x, offsetof(struct elision_config, x), sizeof(#x)-1 }

static const struct tune tunings[] =
  {
    FIELD(retry_lock_busy),
    FIELD(retry_lock_internal_abort),
    FIELD(retry_try_xbegin),
    FIELD(retry_trylock_internal_abort),
    {}
  };

#define PAIR(x) x, sizeof (x)-1

/* It's dangerous to reference anything else here due to IFUNC requirements,
   so we implement all the string functions we need ourself. */

static int
simple_strncmp (const char *a, const char *b, int len)
{
  int i;
  for (i = 0; i < len; i++)
    {
      if (*a != *b)
        return *a - *b;
      if (*a++ == 0 || *b++ == 0)
        break;
    }
  return 0;
}

static int
simple_strtou (const char *s, char **end)
{
  unsigned num = 0;

  while (*s >= '0' && *s <= '9')
    num = (num * 10) + *s++ - '0';
  if (end)
    *(const char **)end = s;
  return num;
}

static void
complain (const char *msg, int len)
{
  INTERNAL_SYSCALL_DECL (err);
  INTERNAL_SYSCALL (write, err, 3, 2, (char *)msg, len);
}

static void
elision_aconf_setup(const char *s)
{
  int i;

  while (*s)
    {
      for (i = 0; tunings[i].name; i++)
	{
	  int nlen = tunings[i].len;

	  if (!simple_strncmp (tunings[i].name, s, nlen) && s[nlen] == ':')
	    {
	      char *end;
	      int val;

	      s += nlen + 1;
	      val = simple_strtou (s, &end);
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
  complain (PAIR("pthreads: invalid PTHREAD_MUTEX syntax\n"));
}

int __rwlock_rtm_enabled attribute_hidden;
int __rwlock_rtm_read_retries attribute_hidden = 3;
int __elision_available attribute_hidden;

#define PAIR(x) x, sizeof (x)-1

static char *
next_env_entry (char first, char ***position)
{
  char **current = *position;
  char *result = NULL;

  while (*current != NULL)
    {
      if ((*current)[0] == first)
	{
	  result = *current;
	  *position = ++current;
	  break;
	}

      ++current;
    }

  return result;
}

static inline void
match (const char *line, const char *var, int len, const char **res)
{
  if (!simple_strncmp (line, var, len))
    *res = line + len;
}

static void
elision_mutex_init (const char *s)
{
  if (!s)
    return;
  if (!simple_strncmp (s, "adaptive", 8) && (s[8] == 0 || s[8] == ':'))
    {
      __pthread_force_elision = __elision_available;
      if (s[8] == ':')
	elision_aconf_setup (s + 9);
    }
  else if (!simple_strncmp (s, "elision", 7) && (s[7] == 0 || s[7] == ':'))
    {
      __pthread_force_elision = __elision_available;
      if (s[7] == ':')
        elision_aconf_setup (s + 8);
    }
  else if (!simple_strncmp (s, "none", 4) && s[4] == 0)
    __pthread_force_elision = 0;
  else
    complain (PAIR("pthreads: Unknown setting for PTHREAD_MUTEX\n"));
}

static void
elision_rwlock_init (const char *s)
{
  if (!s)
    {
      __rwlock_rtm_enabled = __elision_available;
      return;
    }
  if (!simple_strncmp (s, "elision", 7))
    {
      __rwlock_rtm_enabled = __elision_available;
      if (s[7] == ':')
        {
          char *end;
	  int n;

          n = simple_strtou (s + 8, &end);
	  if (end == s + 8)
	    complain (PAIR("pthreads: Bad retry number for PTHREAD_RWLOCK\n"));
          else
	    __rwlock_rtm_read_retries = n;
	}
    }
  else if (!simple_strncmp(s, "none", 4) && s[4] == 0)
    __rwlock_rtm_enabled = 0;
  else
    complain (PAIR("pthreads: Unknown setting for PTHREAD_RWLOCK\n"));
}

static void
elision_init (int argc __attribute__ ((unused)),
	      char **argv  __attribute__ ((unused)),
	      char **environ)
{
  char *envline;
  const char *mutex = NULL, *rwlock = NULL;

  __pthread_force_elision = 1;
  __elision_available = 1;

  while ((envline = next_env_entry ('P', &environ)) != NULL)
    {
      match (envline, PAIR("PTHREAD_MUTEX="), &mutex);
      match (envline, PAIR("PTHREAD_RWLOCK="), &rwlock);
    }

  elision_mutex_init (mutex);
  elision_rwlock_init (rwlock);
}

#ifdef SHARED
# define INIT_SECTION ".init_array"
#else
# define INIT_SECTION ".preinit_array"
#endif

void (*const init_array []) (int, char **, char **)
  __attribute__ ((section (INIT_SECTION), aligned (sizeof (void *)))) =
{
  &elision_init
};

