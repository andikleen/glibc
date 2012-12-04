/* elision-conf.h: Lock elision tunable parameters.
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
#ifndef _ADAPTIVE_CONF_H
#define _ADAPTIVE_CONF_H 1

#include <pthread.h>
#include <cpuid.h>

/* Should make sure there is no false sharing on this */

struct elision_config 
{
  int retry_lock_busy;
  int retry_lock_internal_abort;
  int retry_try_xbegin;
  int retry_trylock_internal_abort;
};

extern struct elision_config __elision_aconf attribute_hidden;

extern __pthread_abort_hook_t __tsx_abort_hook;

extern int __rwlock_rtm_enabled;
extern int __elision_available;

extern void elision_init (char **) attribute_hidden;

/* Use own code to avoid ifunc problems with the main cpuid code */

#define CPUID_FEATURE_RTM (1U << 11)

static inline int 
cpu_has_rtm (void)
{
  if (__get_cpuid_max (0, NULL) >= 7)
    {
      unsigned a, b, c, d;

      __cpuid_count (7, 0, a, b, c, d);
      if (b & CPUID_FEATURE_RTM) 
	return 1;
    }
  return 0;
}

#endif
