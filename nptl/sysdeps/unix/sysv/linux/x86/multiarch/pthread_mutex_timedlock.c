/* Elided IFUNC version of pthread_mutex_timedlock.
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
#include "init-arch.h"

#define ENABLE_ELISION 1
#define pthread_mutex_timedlock __pthread_mutex_timedlock_rtm
#include "nptl/pthread_mutex_timedlock.c"
#undef pthread_mutex_timedlock

libm_ifunc (pthread_mutex_timedlock,
	    (HAS_RTM ? __pthread_mutex_timedlock_rtm
		     : __pthread_mutex_timedlock_nortm))
