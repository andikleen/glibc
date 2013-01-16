/* abort-hook.c: Abort debugging support code.
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
#include <pthreadP.h>
#include "elision-conf.h"

__pthread_abort_hook_t __tsx_abort_hook attribute_hidden;

/* Allow user programs to hook into the abort handler.
   This is useful for debugging situations where you need to get
   information out of a transaction. */

__pthread_abort_hook_t __pthread_set_abort_hook(__pthread_abort_hook_t hook)
{
  __pthread_abort_hook_t old = __tsx_abort_hook;
  __tsx_abort_hook = hook;
  return old;
}
