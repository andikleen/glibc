
Lock elision using TSX is a technique to optimize lock scaling.
It allows to run locks in parallel using hardware support for
a transactional execution mode

See http://software.intel.com/file/41604 for the full
specification.

This patchkit implements a simple adaptive lock elision algorithm based
on RTM. It enables elision for the pthread mutexes and rwlocks.
The algorithm keeps track whether a mutex successfully elides or not,
and stops eliding for some time when it is not.

When the CPU supports RTM the elision path is automatically tried,
otherwise any elision is disabled.

The adaptation algorithm and its tuning is currently preliminary.

The code adds some checks to the lock fast paths. I originally experimented
with IFUNC to only add this on systems supporting RTM and with elision
enabled. But I ran into various problems with IFUNC, so I ended up with the
simpler if approach.

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

PTHREAD_MUTEX=adaptive
Use adaptive RTM based mutexes. Default when CPUID indicated RTM support.
In addition these mutexes can be configured like this
PTHREAD_MUTEX=adaptive:retry_lock_busy=10,retry_lock_internal_abort=20 ...
See below for a description of the fields.

PTHREAD_MUTEX=none
Don't use HLE mutexes even if the CPU has RTM support (unless explicitly
set by the program)

pthread mutex kinds:

pthread allows to set specific lock types for individual mutexes. This
can be used to overwrite the default mutex type for a specific
mutex. This can be only done by modifying the program

Available new types are:

PTHREAD_MUTEX_HLE_ADAPTIVE_NP           (please don't confuse with PTHREAD_MUTEX_ADAPTIVE_NP)
Adaptive RTM mutex even when otherwise disabled (but still elides)

PTHREAD_MUTEX_TIMED_NONHLE_NP
Unconditional non HLE/RTM mutex. This is useful to force some mutexes to
never attempt a transaction if it's known to be not worthwhile.

(note these names may change still, I'm not very happy with them)

Tunables in PTHREAD_MUTEX:

Tunables (settable using the PTHREAD_MUTEX environment variable, see above):
The current numbers do work reasonably in pin, but are likely not very good.

retry_lock_busy
How often to not attempt a transaction when the lock is seen as busy.

retry_lock_internal_abort
How often to not attempt a transaction after an internal abort is seen.

retry_try_xbegin
How often to retry the transaction on external aborts.

Changes with the RTM mutexes:
-----------------------------
Lock elision in pthreads is generally compatible with existing programs.
There are some obscure exceptions, which are expected to be uncommon:

- A broken program that unlocks a free lock will crash.
  There are ways around this with some tradeoffs (more code in hot paths)
  This will also happen on systems without RTM with the patchkit.
  I'm still undecided on what approach to take here; have to wait for testing reports.
- pthread_mutex_destroy of a lock mutex will not return EBUSY but 0.
- mutex appears free when elided.
  pthread_mutex_lock(mutex);
  if (pthread_mutex_trylock(mutex) != 0) do_something
  will not do something when the lock elided.
  However note that if the check is an assert it works as expected because the
  assert failure aborts and the region is re-executed non transactionally,
  with the old behaviour.
- There's also a similar situation with trylock outside the mutex,
  "knowing" that the mutex must be held due to some other condition.
  In this case an assert failure cannot be recovered. This situation is
  usually an existing bug in the program.
- Same applies to the rwlocks. Some of the return values changes
  (for example there is no EDEADLK for an elided lock, unless it aborts.
   However when elided it will also never deadlock of course)
- Timing changes, so broken programs that make assumptions about specific timing
  may expose already existing latent problems.  Note that these broken programs will
  break in other situations too (loaded system, new faster hardware, compiler
  optimizations etc.)

Currently elision is enabled by default on systems that support RTM,
unless explicitely disabled either in the program or by the user.
Given more experience we can decide if that is a good idea, or if it
should be opt-in.

Limitations not fixable:
------------------------
- Adaptive enabled mutexes don't track the owner, so pthread_mutex_destroy
will not detect a busy mutex.
- Trylock on a already guaranteed to be locked lock will succeed

Limitations that may be fixable (but it's unclear if it's worth it):
-------------------------------------------------------------------
- Unlocking an unlocked mutex will result in a crash currently
(see above)
- No elision support for recursive, error check mutexes
Recursive may be possible, error check is unlikely
- Some obscure cases can also fallback to non elision
- Internal locks in glibc (like malloc or stdio) do not elide at this
  point.

To be fixed:
------------
- The default tuning parameters may be revised.
- Unclear where to document the environment variables
- Condition variables don't use elision so far

