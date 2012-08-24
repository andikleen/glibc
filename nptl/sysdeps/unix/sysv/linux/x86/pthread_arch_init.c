/* Copyright (C) 2012 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Andi Kleen

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */
#include <stdlib.h>
#include <pthreadP.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <cpuid.h>
#include <unistd.h>

#include "adaptive-conf.h"

/* Work around for __environ not initialized early enough (FIXME) */
static char *brute_force_getenv (const char *name, char *buf, int bufsize)
{
  int fd, n, nlen;
  char *p;

  if (__environ)
    return getenv (name);
  fd = __open ("/proc/self/environ", O_RDONLY);
  if (fd < 0)
    return NULL;
  n = __read (fd, buf, bufsize);
  __close(fd);
  if (n <= 0)
    return NULL;
  nlen = strlen (name);
  p = buf;
  while (p < buf + n) 
    {      
      p = memmem (p, buf + n - p, name, nlen);
      if (p == NULL)
	break;
      if ((p == buf || p[-1] == 0) && p + nlen + 1 < buf + n && p[nlen] == '=')
	return p + nlen + 1;	  
      p++;
    }	  
  return NULL;
}

#define CPUID_FEATURE_RTM (1U << 11)

static int 
cpu_has_rtm (void)
{
  if (__get_cpuid_max (0, NULL) >= 7)
    {
      unsigned a, b, c, d;

      __cpuid_count (7, 0, a, b, c, d);
      if (b & CPUID_FEATURE_RTM) 
	return 1;
    }
  return 0;
}

#define PAIR(x) x, sizeof (x)-1

static void __attribute__((constructor)) 
pthread_arch_init (void)
{
  char buf[16 * 1024]; /* FIXME */
  char *s;

  /* Assume adaptive is good enough that we can make it default 
     when the CPU supports it. */
  if (cpu_has_rtm ()) 
    __pthread_force_hle = 2;
  s = brute_force_getenv ("PTHREAD_MUTEX", buf, sizeof buf);
  if (!s)
    return;
  if (!strncmp (s, "adaptive", 8) && (s[8] == 0 || s[8] == ':'))
    {
      __pthread_force_hle = 2;
      if (s[8] == ':')
	hle_aconf_setup (s + 9);
    }
  else if (!strcmp (s, "none"))
    __pthread_force_hle = 0;
  else 
    __write (2, PAIR("pthreads: Unknown setting for PTHREAD_MUTEX\n"));
}
