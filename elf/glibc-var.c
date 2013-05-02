/* Fast access to GLIBC_* environment variables, without having to walk
   the environment multiple times.
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

#include <string.h>
#include <glibc-var.h>

struct glibc_var _dl_glibc_var[] = 
{
  [GLIBC_VAR_MUTEX]  = { "MUTEX",  5, NULL },
  [GLIBC_VAR_RWLOCK] = { "RWLOCK", 6, NULL },
  /* Add more GLIBC_ variables here */
  [GLIBC_VAR_MAX] =    { NULL, 0, NULL }
};

internal_function void
__record_glibc_var (char *name, int len, char *val)
{
  int i;

  for (i = 0; _dl_glibc_var[i].name; i++)
    {
      struct glibc_var *v = &_dl_glibc_var[i];

      if (len == v->len && !memcmp (v->name, name, v->len))
        {
	  v->val = val;
	  break;
	}
    }
  /* Ignore unknown GLIBC_ variables. */
}

#ifndef SHARED

/* If SHARED the env walk is shared with rtld.c */

static char *
next_env_entry (char first, char ***position)
{
  char **current = *position;
  char *result = NULL;

  while (*current != NULL)
    {
      if ((*current)[0] == first)
	{
	  result = *current;
	  *position = ++current;
	  break;
	}

      ++current;
    }

  return result;
}

/* May be called from libpthread */

void
__glibc_var_init (int argc __attribute__ ((unused)),
		  char **argv  __attribute__ ((unused)),
		  char **environ)
{
  char *envline;
  static int initialized;

  if (initialized)
    return;
  initialized = 1;

  while ((envline = next_env_entry ('G', &environ)) != NULL)
    {
      if (envline[1] == 'L' && envline[2] == 'I' && envline[3] == 'B'
	  && envline[4] == 'C' && envline[5] == '_')
	{
	  char *e = envline + 6;
	  while (*e && *e != '=')
	    e++;
	  if (*e == 0)
	    continue;
	  __record_glibc_var (envline + 6, e - (envline + 6), e + 1);
	}
    }
}

void (*const __glibc_var_init_array []) (int, char **, char **)
  __attribute__ ((section (".preinit_array"), aligned (sizeof (void *)))) =
{
  &__glibc_var_init
};
#endif
