/* Copyright (C) 2005, 2006, 2007, 2012 Free Software Foundation, Inc.
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

/* We test the code undef conditions outside of glibc.  */
#undef _LIBC

#include <pthread.h>

/* Test initializers for elided locks */

pthread_mutex_t mtx_timed_elision = PTHREAD_MUTEX_INIT_NP(PTHREAD_MUTEX_TIMED_NP|
							  PTHREAD_MUTEX_ELISION_NP);
pthread_mutex_t mtx_timed_no_elision = PTHREAD_MUTEX_INIT_NP(PTHREAD_MUTEX_TIMED_NP|
							     PTHREAD_MUTEX_NO_ELISION_NP);
pthread_mutex_t mtx_adaptive_elision = PTHREAD_MUTEX_INIT_NP(PTHREAD_MUTEX_ADAPTIVE_NP|
							     PTHREAD_MUTEX_ELISION_NP);
pthread_mutex_t mtx_adaptive_no_elision = PTHREAD_MUTEX_INIT_NP(PTHREAD_MUTEX_ADAPTIVE_NP|
								PTHREAD_MUTEX_NO_ELISION_NP);

int
main (void)
{
  if (mtx_timed_elision.__data.__kind !=
      (PTHREAD_MUTEX_TIMED_NP|PTHREAD_MUTEX_ELISION_NP))
    return 1;
  if (mtx_timed_no_elision.__data.__kind !=
      (PTHREAD_MUTEX_TIMED_NP|PTHREAD_MUTEX_NO_ELISION_NP))
    return 2;
  if (mtx_adaptive_elision.__data.__kind !=
      (PTHREAD_MUTEX_ADAPTIVE_NP|PTHREAD_MUTEX_ELISION_NP))
    return 3;
  if (mtx_adaptive_no_elision.__data.__kind !=
      (PTHREAD_MUTEX_ADAPTIVE_NP|PTHREAD_MUTEX_NO_ELISION_NP))
    return 4;
  return 0;
}
