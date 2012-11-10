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
#ifndef _ELISION_CONF_H
#define _ELISION_CONF_H 1

#include <pthread.h>
#include <cpuid.h>
#include <time.h>

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

extern int __pthread_mutex_timedlock_nortm (pthread_mutex_t *mutex, const struct timespec *);
extern int __pthread_mutex_timedlock_rtm (pthread_mutex_t *mutex, const struct timespec *);
extern int __pthread_mutex_timedlock (pthread_mutex_t *mutex, const struct timespec *);
extern int __pthread_mutex_lock_nortm (pthread_mutex_t *mutex);
extern int __pthread_mutex_lock_rtm (pthread_mutex_t *mutex);
extern int __pthread_mutex_lock (pthread_mutex_t *mutex);
extern int __pthread_mutex_trylock_nortm (pthread_mutex_t *);
extern int __pthread_mutex_trylock_rtm (pthread_mutex_t *);
extern int __pthread_mutex_trylock (pthread_mutex_t *);

#define SUPPORTS_ELISION 1
#define SUPPORTS_ABORT_HOOK 1

#endif
