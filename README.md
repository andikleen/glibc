TSX lock elision for glibc v4

History:
v1: Initial post
v2: Remove IFUNC use.
v3: Nested trylock aborts by default now.
    Trylock enables elision on non-upgraded lock.
    Various bug fixes.
    New initializers, remove explicit new lock types in external interface.
    Add example of abort hook to manual.
    Fix bugs and clean up the configuration parser.
    Fix bug in lock busy handling.
    Fix tst-elision2 which was disabled by mistake earlier.
v4:
    Add missing tst-elision3 file, but not it's tst-elision2
    Remove abort hook and related code.
    Use symbolic abort codes

Lock elision using TSX is a technique to optimize lock scaling.
It allows to run existing locks in parallel using hardware memory
transactions. New instructions (RTM) are used to control
memory transactions.

The full series is available at 
http://github.com/andikleen/glibc
git://github.com/andikleen/glibc rtm-devel6

An overview of lock elision is available at
http://halobates.de/adding-lock-elision-to-linux.pdf

LWN article on the topic:
https://lwn.net/Articles/534758/ 
(still behind paywall for this week)

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

I cannot post performance numbers at this point.

The user can also tune this by setting the mutex type and environment
variables.

The lock transactions have a abort hook mechanism to hook into the abort
path. This is quite useful for some debugging, so I kept this
functionality.

The mutexes can be configured at runtime with the PTHREAD_MUTEX
environment variable.  This will force a specific lock type for all
mutexes in the program that do not have another type set explicitly.
This can be done without modifying the program.

Currently elision is enabled by default on systems that support RTM,
unless explicitely disabled either in the program or by the user.
Given more experience we can decide if that is a good idea, or if it
should be opt-in.

Limitations that may be fixable (but it's unclear if it's worth it):
-------------------------------------------------------------------
- Adaptive enabled mutexes don't track the owner, so pthread_mutex_destroy
will not detect a busy mutex.
- Unlocking an unlocked mutex will result in a crash currently
(see above)
- No elision support for recursive, error check mutexes
Recursive may be possible, error check is unlikely
- Some obscure cases can also fallback to non elision
- Internal locks in glibc (like malloc or stdio) do not elide at this
  point.

Changing these semantics would be possible, but has some runtime cost. Currently
I decided to not do any expensive changes, but wait for more testing feedback.

To be fixed:
------------
- The default tuning parameters may be revised.

