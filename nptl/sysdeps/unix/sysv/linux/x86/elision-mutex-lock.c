/* elision-mutex-lock.c: Elided IFUNC version of pthread_mutex_lock.
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
#include "elision-conf.h"
#include "force-elision.h"
#ifdef SHARED
#define ENABLE_ELISION 1
#define __pthread_mutex_lock __pthread_mutex_lock_rtm
#else
/* For static builds simply default to the RTM version, but we must
   check CPUID explicitely at runtime. For the shared build IFUNC
   enforces this. */
#define ENABLE_ELISION (__elision_available != 0)
#endif
#include "nptl/pthread_mutex_lock.c"
#undef __pthread_mutex_lock

#ifdef SHARED
extern int __pthread_mutex_lock_nortm (pthread_mutex_t *mutex);
extern int __pthread_mutex_lock (pthread_mutex_t *mutex);
/* Use own CPUID code because of ordering problems with the main query */
libm_ifunc (__pthread_mutex_lock, cpu_has_rtm () ?
		__pthread_mutex_lock_rtm : __pthread_mutex_lock_nortm)
strong_alias (__pthread_mutex_lock, pthread_mutex_lock)
/* hidden_def (__pthread_mutex_lock) */
#endif
