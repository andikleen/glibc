/* Fast access to GLIBC_* environment variables, without having to walk
   the environment. Register new ones in in elf/glibc-var.c
   Copyright (C) 2013 Free Software Foundation, Inc.

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
#ifndef _GLIBC_VAR_H
#define _GLIBC_VAR_H 1

#include <libc-symbols.h>

enum
{
 GLIBC_VAR_MUTEX,
 GLIBC_VAR_RWLOCK,
 GLIBC_VAR_MAX
};

struct glibc_var
{
  const char *name;
  int len; 
  char *val;
};

extern struct glibc_var _dl_glibc_var[];
extern void __record_glibc_var (char *name, int len, char *val) internal_function;

/* Call this if you're in a constructor that may run before glibc-var's */
#ifndef SHARED
extern void __glibc_var_init (int ac, char **av, char **env);
#else
/* For shared this is always done in the dynamic linker early enough. */
#define __glibc_var_init(a,b,c) do {} while(0)
#endif

#endif
