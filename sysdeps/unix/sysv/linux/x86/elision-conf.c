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
#include <sys/mman.h>
#include <sys/fcntl.h>

#define PAIR(x) x, sizeof (x)-1

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

#define X86_PAGE_SIZE 4096
#define ENV_VAR_WHITELIST "/etc/glibc-env.cfg"

/* Does specific M match LINE before END? Write length back to LENP.  */

static bool
match (char *line, char *end, const char *m, int *lenp)
{
  int len = strlen (m);

  *lenp = 0;
  if (len > end - line)
    return false;
  if (!strncmp (line, m, len))
    {
      *lenp = len;
      return true;
    }
  return false;
}

/* INTERNAL_SYSCALL does not support 6 argument calls on i386, so do our own
   mmap stub here. addr and offset are hardcoded to 0 to simplify the
   assembler.  */

static inline void *
mmap_00(size_t length, int prot, int flags, int fd)
{
  void *ret;

#ifdef __i386__
  asm(
     "push %%ebx\n\t"
     "push %%ebp\n\t"
     "mov %1,%%eax\n\t" /* syscall number */
     "xor %%ebx,%%ebx\n\t" /* addr */
     "xor %%ebp,%%ebp\n\t" /* offset */
     "int $0x80\n\t"
     "pop %%ebp\n\t"
     "pop %%ebx\n\t"
     : "=a" (ret)
     : "i" (__NR_mmap2), "c" (length), "d" (prot), "S" (flags), "D" (fd));
#else
  INTERNAL_SYSCALL_DECL (err);
  ret = (void *)INTERNAL_SYSCALL (mmap, err, 6, NULL, length,
				  prot, flags, fd, 0);
#endif
  return ret;
}

/* Check extra file to see if we're allowed to use environment variables.
   Should be moved elsewhere if extended for other purposes.  */

static bool
whitelist_env_var (void)
{
  int fd;
  struct stat st;
  INTERNAL_SYSCALL_DECL (err);
  bool ok = false;

#ifndef ENABLE_ELISION_TUNE_WHITELIST
  return true;
#endif

  fd = INTERNAL_SYSCALL (open, err, 2, ENV_VAR_WHITELIST, O_RDONLY);
  if (fd < 0)
    return false;
  if (INTERNAL_SYSCALL (fstat, err, 2, fd, &st) >= 0)
    {
      size_t size = (st.st_size + X86_PAGE_SIZE - 1) & ~(X86_PAGE_SIZE - 1);
      char *map = mmap_00 (size, PROT_READ, MAP_SHARED, fd);
      if (map != (char *)-1L)
	{
	  char *line;
	  char *end = map + st.st_size;

	  for (line = map; line < end; )
	    {
	      while (line < end && (*line == ' ' || *line == '\t' || *line == '\n'))
		line++;
	      if (line >= end)
		break;
	      if (*line == '#')
		{
		  while (line < end && *line != '\n')
		    line++;
		}
	      else if (*line != '\n')
		{
		  int len = 0;

		  if (match (line, end, "allow_elision_tuning", &len))
		    ok = true;
		  else if (match (line, end, "disallow_elision_tuning", &len))
		    ok = false;
		  else
		    {
		      complain (PAIR ("Unknown configuration specifier in " ENV_VAR_WHITELIST "\n"));
		      len = end - line;
		      if (len > 20)
			len = 20;
		      complain (line, len);
		      complain (PAIR ("\n"));
		      ok = false;
		      break;
		    }
		  line += len;
		  while (line < end && (*line == ' ' || *line == '\t' || *line == '\n'))
		    line++;
		}
	    }
	  INTERNAL_SYSCALL (munmap, err, 2, map, size);
	}
    }
  INTERNAL_SYSCALL (close, err, 1, fd);
  return ok;
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

  if (whitelist_env_var ())
    {
      elision_mutex_init (_dl_glibc_var[GLIBC_VAR_PTHREAD_MUTEX].val);
      elision_rwlock_init (_dl_glibc_var[GLIBC_VAR_PTHREAD_RWLOCK].val);
    }
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
