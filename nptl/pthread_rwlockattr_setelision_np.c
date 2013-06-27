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

#ifndef ENABLE_ELISION
# define ENABLE_ELISION 0
#endif

int
pthread_rwlockattr_setelision_np (pthread_rwlockattr_t *attr, int flag)
{
  struct pthread_rwlockattr *iattr;

  iattr = (struct pthread_rwlockattr *) attr;

  if (ENABLE_ELISION)
    {
      if (flag == PTHREAD_MUTEX_ELISION_ENABLE_NP)
        iattr->rw_elision = 1;
      else if (flag == PTHREAD_MUTEX_ELISION_DISABLE_NP)
        iattr->rw_elision = -1;
      else
	return EINVAL;
    }
  return 0;
}
