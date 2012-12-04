/* elision-mutex-trylock.c: Elided IFUNC version of pthread_mutex_trylock.
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
#define __pthread_mutex_trylock __pthread_mutex_trylock_rtm
#else
/* For static builds simply default to the rtm version. */
#define ENABLE_ELISION (__elision_available != 0)
#endif
#include "nptl/pthread_mutex_trylock.c"
#undef __pthread_mutex_trylock

#ifdef SHARED
extern int __pthread_mutex_trylock_nortm (pthread_mutex_t *);
extern int __pthread_mutex_trylock (pthread_mutex_t *);
libm_ifunc (__pthread_mutex_trylock, cpu_has_rtm () ?
	    __pthread_mutex_trylock_rtm : __pthread_mutex_trylock_nortm);
strong_alias(__pthread_mutex_trylock, pthread_mutex_trylock);
/* hidden_def (__pthread_mutex_trylock); */
#endif
