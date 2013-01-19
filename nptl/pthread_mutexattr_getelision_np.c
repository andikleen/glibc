/* Copyright (C) 2013 Free Software Foundation, Inc.
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

#include <pthreadP.h>

int
pthread_mutexattr_getelision_np (const pthread_mutexattr_t *attr, int *elision)
{
  const struct pthread_mutexattr *iattr;

  iattr = (const struct pthread_mutexattr *) attr;

  if (iattr->mutexkind & PTHREAD_MUTEX_ELISION_NP)
    *elision = PTHREAD_ELISION_ALWAYS;
  else if (iattr->mutexkind & PTHREAD_MUTEX_NO_ELISION_NP)
    *elision = PTHREAD_ELISION_NEVER;
  else
    *elision = PTHREAD_ELISION_DEFAULT;

  return 0;
}
