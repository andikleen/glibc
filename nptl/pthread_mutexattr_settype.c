/* Copyright (C) 2002-2013 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@redhat.com>, 2002.

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

#include <errno.h>
#include <pthreadP.h>
#include <shlib-compat.h>

static int
pthread_mutexattr_settype_worker (pthread_mutexattr_t *attr, int kind)
{
  struct pthread_mutexattr *iattr;
  int mkind = kind & ~PTHREAD_MUTEX_ELISION_FLAGS_NP;

  if (mkind < PTHREAD_MUTEX_TIMED_NP || mkind > PTHREAD_MUTEX_NORMAL)
    return EINVAL;
  /* Cannot set multiple flags.  */
  if ((kind & PTHREAD_MUTEX_ELISION_FLAGS_NP) == PTHREAD_MUTEX_ELISION_FLAGS_NP)
    return EINVAL;

  /* When a NORMAL mutex is explicitly specified, default to no elision
     to satisfy POSIX's deadlock requirement. Also convert the NORMAL
     type to DEFAULT, as the rest of the lock library doesn't have
     the code paths for them.  */
  if (mkind == PTHREAD_MUTEX_NORMAL)
    {
      kind = PTHREAD_MUTEX_DEFAULT | (kind & PTHREAD_MUTEX_ELISION_FLAGS_NP);
      if ((kind & PTHREAD_MUTEX_ELISION_FLAGS_NP) == 0)
        kind |= PTHREAD_MUTEX_NO_ELISION_NP;
    }

  iattr = (struct pthread_mutexattr *) attr;

  iattr->mutexkind = (iattr->mutexkind & PTHREAD_MUTEXATTR_FLAG_BITS) | kind;

  return 0;
}

int
__pthread_mutexattr_settype_new (pthread_mutexattr_t *attr, int kind)
{
  return pthread_mutexattr_settype_worker (attr, kind);
}

weak_alias (__pthread_mutexattr_settype_new, __new_pthread_mutexattr_setkind_np)
strong_alias (__pthread_mutexattr_settype_new, __new_pthread_mutexattr_settype)

versioned_symbol (libpthread, __new_pthread_mutexattr_setkind_np,
		  pthread_mutexattr_setkind_np, GLIBC_2_18);
versioned_symbol (libpthread, __new_pthread_mutexattr_settype,
		  pthread_mutexattr_settype, GLIBC_2_18);
versioned_symbol (libpthread, __pthread_mutexattr_settype_new,
		  __pthread_mutexattr_settype, GLIBC_2_18);

#if SHLIB_COMPAT (libpthread, GLIBC_2_0, GLIBC_2_18) \
    || SHLIB_COMPAT (libpthread, GLIBC_2_1, GLIBC_2_18) \

int
attribute_compat_text_section
__pthread_mutexattr_settype_old (pthread_mutexattr_t *attr, int kind)
{
  /* Force no elision for the old ambigious DEFAULT/NORMAL
     kind.  */
  if (kind == PTHREAD_MUTEX_DEFAULT)
    kind |= PTHREAD_MUTEX_NO_ELISION_NP;
  return pthread_mutexattr_settype_worker (attr, kind);
}


# if SHLIB_COMPAT (libpthread, GLIBC_2_0, GLIBC_2_18)
weak_alias (__pthread_mutexattr_settype_old, __old_pthread_mutexattr_setkind_np)
strong_alias (__pthread_mutexattr_settype_old, __old_pthread_mutexattr_settype)
compat_symbol (libpthread,
	       __old_pthread_mutexattr_setkind_np,
	       pthread_mutexattr_setkind_np,
	       GLIBC_2_0);
compat_symbol (libpthread,
	       __old_pthread_mutexattr_settype,
	       __pthread_mutexattr_settype,
	       GLIBC_2_0);
# endif
# if SHLIB_COMPAT (libpthread, GLIBC_2_1, GLIBC_2_18)
compat_symbol (libpthread,
	       __pthread_mutexattr_settype_old,
	       pthread_mutexattr_settype,
	       GLIBC_2_1);
# endif
#endif
