TSX lock elision for glibc v13

Proposed NEWS entry:

* Added support for TSX lock elision for pthread mutexes and read/write
  locks on i386 and x86-64.
  This allows to improve lock scaling of existing programs.
  By default this is not enabled, unless there is an 
  explicitly elided lock type in the program.
  When the --enable-lock-elision=on parameter is specified at configure
  time lock elision will be enabled by default for all locks.
  See the manual for more details.

History:
+ v1: Initial post
+ v2: Remove IFUNC use.
+ v3: Nested trylock aborts by default now.
    Trylock enables elision on non-upgraded lock.
    Various bug fixes.
    New initializers, remove explicit new lock types in external interface.
    Add example of abort hook to manual.
    Fix bugs and clean up the configuration parser.
    Fix bug in lock busy handling.
    Fix tst-elision2 which was disabled by mistake earlier.
+ v4: (= rtm-devel6)
    Add missing tst-elision3 file, but not it's tst-elision2
    Remove abort hook and related code.
    Use symbolic abort codes
+ v5: (= rtm-devel7)
    Rebased to current master.
    Use GLIBC_* prefixes for environment variables.
    Merge environment scan with dynamic linker
    Fix CPUID id that broke earlier.
    Minor cleanups.
+ v6: Add review feedback on glibc-var.c
+ v7: Merge with partially merged patch and fix ChangeLog.
+ v8: Rebase to current master.
    Add --enable-lock-elision and disable by default.
    Address review feedback.
    Make environment variables extra verbose.
    Various test suite fixes.
+ v9: Fix test suite for systems without TSX.
+ v10: Manual fixes
    Minor white space changes
    Rename some tuning fields from retry to skip to be more logical
    Use correct lock for condvar lock relock when an adaptive lock is used.
    Fix a problem that the wrong field was used for elision of adaptive locks.
+ v11:
    Environment variables split out into separate patches.
    Remove "adaptive" alias.
    Don't rely on the environment variables for the test suite. This mainly
    affected tst-mutex8, which tests some non POSIX behaviour now only for the subtests
    that explicitely disable elision (this gives the same coverage overall)
    Various review feedback addressed, including a bug in non smp mode with adaptive locks
    (thanks Torvald!)
    Various white space and comment change.
    __elided -> __rw_elision for read locks and some other identifier changes.
    Clarify delayed deadlock behaviour in manual.
+ v12:
    Further clarification in the manual for deadlock behaviour.
    Don't enable elision by default for suid programs.
    Support --enable-lock-elision=on/off
    Add no_elision alias for none
+ v13:
    Add new number for PTHREAD_MUTEX_NORMAL
    Disable elision for PTHREAD_MUTEX_NORMAL mutexes in pthread_mutexattr_settype()
    Add compat code for old glibcs disabling on PTHREAD_MUTEX_DEFAULT/NORMAL
    Remove PTHREAD_MUTEX_INIT_NP() initializer for better binary compatibility.
    This also allows enforcing no elision flags ever set when RTM is not supported
    at runtime, so the fast path does not need to check for this.
    Some tweaks shave off some more instructions off in pthread_mutex_lock/unlock

Lock elision using TSX is a technique to optimize lock scaling.
It allows to run existing locks in parallel using hardware memory
transactions. New instructions (RTM) are used to control
memory transactions.

The full series is available at 
http://github.com/andikleen/glibc
git://github.com/andikleen/glibc rtm-devel8

An overview of lock elision is available at
http://halobates.de/adding-lock-elision-to-linux.pdf

LWN article on the topic:
https://lwn.net/Articles/534758/ 

See http://software.intel.com/file/41604 for the full
TSX specification. Running TSX requires either new hardware with TSX
support, or using the SDE emulator 
http://software.intel.com/en-us/articles/intel-software-development-emulator/

This patchkit implements a simple adaptive lock elision algorithm based
on RTM. It enables elision for the pthread mutexes and rwlocks.
The algorithm keeps track whether a mutex successfully elides or not,
and stops eliding for some time when it is not.

When the CPU supports RTM the elision path is automatically tried,
otherwise any elision is disabled.

The adaptation algorithm and its tuning is currently preliminary.

The user can also tune this by setting the mutex type and environment
variables.

The mutexes can be configured at runtime with the GLIBC_PTHREAD_MUTEX
environment variable.  This will force a specific lock type for all
mutexes in the program that do not have another type set explicitly.
This can be done without modifying the program.

For read/write locks the default be enable/disable using the
GLIBC_PTHREAD_RWLOCK environment variable.

GLIBC_PTHREAD_MUTEX=elision GLIBC_PTHREAD_RWLOCK=elision program

Currently lock elision is not forced by default, unless explicitely
set. This default can be changed at glibc compile time by 
adding the --enable-lock-elision parameter to configure.
Some packagers may chose to change this default.

Limitations that may be fixable (but it's unclear if it's worth it):
-------------------------------------------------------------------
- Adaptive enabled mutexes don't track the owner, so pthread_mutex_destroy
will not detect a busy mutex.
- Unlocking an unlocked mutex will result in a crash currently
(see above)
- No elision support for recursive, error check mutexes
Recursive may be possible, error check is unlikely
- Some cases can also fallback to non elision. Most are obscure,
except for the condition variables.
- Internal locks in glibc (like malloc or stdio) do not elide.

Changing some these semantics would be possible, but has some runtime cost. Currently
I decided to not do any expensive changes, but wait for more testing feedback.

To be fixed:
------------
- The default tuning parameters may be revised.
- Condition variables ought to be elided too.
- Adaptive rwlocks.
- Better algorithm for adaptive elided mutexes.

