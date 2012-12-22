/* __pthread_initialize_minimal_internal () has

   GL(dl_rtld_lock_recursive) = (void *) __pthread_mutex_lock;

   This doesn't work with hidden IFUNC function.  See

   http://www.sourceware.org/bugzilla/show_bug.cgi?id=14961

   for details.  The work around is not to mark __pthread_mutex_lock
   hidden.  It also helps x86-64 since we now use the real function
   address at run-time, instead of its PLT entry.  */

#define __pthread_mutex_lock __GI___pthread_mutex_lock

#include <nptl/nptl-init.c>
