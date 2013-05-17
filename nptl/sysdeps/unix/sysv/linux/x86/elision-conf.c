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
#include <init-arch.h>
#include <elision-conf.h>
#include <unistd.h>

struct elision_config __elision_aconf =
  {
    .skip_lock_busy = 3,
    .skip_lock_internal_abort = 3,
    .retry_try_xbegin = 3,
    .skip_trylock_internal_abort = 3,
  };

/* Elided rwlock toggle.  */

int __rwlock_rtm_enabled attribute_hidden;

/* Retries for elided rwlocks.  */

int __rwlock_rtm_read_retries attribute_hidden = 3;

/* Global elision check switch.  */

int __elision_available attribute_hidden;

/* Force elision for all new locks.  */

int __pthread_force_elision attribute_hidden;

/* Initialize elison.  */

static void
elision_init (int argc __attribute__ ((unused)),
	      char **argv  __attribute__ ((unused)),
	      char **environ)
{
  __elision_available = HAS_RTM;
#ifdef ENABLE_LOCK_ELISION
  __pthread_force_elision = __libc_enable_secure ? 0 : __elision_available;
  __rwlock_rtm_enabled = __libc_enable_secure ? 0 : __elision_available;
#endif
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
