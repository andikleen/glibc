/* Copyright (C) 2002-2012 Free Software Foundation, Inc.
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

#include <sysdep.h>
#include <lowlevellock.h>
#include <lowlevelrwlock.h>
#include <pthread-errnos.h>
#include <kernel-features.h>
#include <stap-probe.h>
#include <hle.h>

	.text

	.globl	__pthread_rwlock_rdlock
	.type	__pthread_rwlock_rdlock,@function
	.align	16
__pthread_rwlock_rdlock:
	cfi_startproc

	LIBC_PROBE (rdlock_entry, 1, %rdi)

#ifdef ENABLE_ELISION
	cmpq  $0,ELIDING(%rdi)
	js    not_elided_rdlock
	jnz   2f
	/* zero: use default */

	cmpl  $0,__rwlock_rtm_enabled(%rip)
	jz    not_elided_rdlock

2:
	mov   __rwlock_rtm_read_retries(%rip),%esi

try_trans_rdlock:
	XBEGIN abort_rdlock

	/* Lock reader/writer free? */
	cmpl  $0,WRITER(%rdi)
	jnz   1f
	cmpl  $0,NR_READERS(%rdi)
	jnz   1f

	/* Lock is free. Run with transaction */
	xor   %eax,%eax
	ret

	/* Lock is not free. Run */
1:	XABORT 0xff
	jmp not_elided_rdlock

	/* Abort happened. */
abort_rdlock:
	/* Call the abort hook */
	cmpq  $0,__tsx_abort_hook(%rip)
	jz    1f

	push  %rdi
	cfi_adjust_cfa_offset(8)
	push  %rsi
	cfi_adjust_cfa_offset(8)
        push  %rax
	cfi_adjust_cfa_offset(8)
	mov   %eax,%edi
	call  *__tsx_abort_hook(%rip)
	pop   %rax
	cfi_adjust_cfa_offset(-8)
	pop   %rsi
	cfi_adjust_cfa_offset(-8)
	pop   %rdi
	cfi_adjust_cfa_offset(-8)

1:

	testl $_XABORT_CONFLICT,%eax
	jz    not_elided_rdlock

	/* For a reader that aborts due a conflict retry speculation
	   a limited number of times. This way when some reader aborts
	   because the reader count is written the other readers will
	   still elide, at the cost of retrying the speculation. */

	dec   %esi
	jnz   try_trans_rdlock

	/* Otherwise we just fall back directly to the lock.
	   Here's the place to add more adaptation. */

not_elided_rdlock:
#endif
	xorq	%r10, %r10

	/* Get the lock.  */
	movl	$1, %esi
	xorl	%eax, %eax
	LOCK
#if MUTEX == 0
	cmpxchgl %esi, (%rdi)
#else
	cmpxchgl %esi, MUTEX(%rdi)
#endif
	jnz	1f

2:	movl	WRITER(%rdi), %eax
	testl	%eax, %eax
	jne	14f
	cmpl	$0, WRITERS_QUEUED(%rdi)
	je	5f
	cmpl	$0, FLAGS(%rdi)
	je	5f

3:	incl	READERS_QUEUED(%rdi)
	je	4f

	movl	READERS_WAKEUP(%rdi), %edx

	LOCK
#if MUTEX == 0
	decl	(%rdi)
#else
	decl	MUTEX(%rdi)
#endif
	jne	10f

11:
#ifdef __ASSUME_PRIVATE_FUTEX
	movl	$FUTEX_PRIVATE_FLAG|FUTEX_WAIT, %esi
	xorl	PSHARED(%rdi), %esi
#else
# if FUTEX_WAIT == 0
	movl	PSHARED(%rdi), %esi
# else
	movl	$FUTEX_WAIT, %esi
	orl	PSHARED(%rdi), %esi
# endif
	xorl	%fs:PRIVATE_FUTEX, %esi
#endif
	addq	$READERS_WAKEUP, %rdi
	movl	$SYS_futex, %eax
	syscall

	subq	$READERS_WAKEUP, %rdi

	/* Reget the lock.  */
	movl	$1, %esi
	xorl	%eax, %eax
	LOCK
#if MUTEX == 0
	cmpxchgl %esi, (%rdi)
#else
	cmpxchgl %esi, MUTEX(%rdi)
#endif
	jnz	12f

13:	decl	READERS_QUEUED(%rdi)
	jmp	2b

5:	xorl	%edx, %edx
	incl	NR_READERS(%rdi)
	je	8f
9:	LOCK
#if MUTEX == 0
	decl	(%rdi)
#else
	decl	MUTEX(%rdi)
#endif
	jne	6f
7:

	movq	%rdx, %rax
	retq

1:	movl	PSHARED(%rdi), %esi
#if MUTEX != 0
	addq	$MUTEX, %rdi
#endif
	callq	__lll_lock_wait
#if MUTEX != 0
	subq	$MUTEX, %rdi
#endif
	jmp	2b

14:	cmpl	%fs:TID, %eax
	jne	3b
	/* Deadlock detected.  */
	movl	$EDEADLK, %edx
	jmp	9b

6:	movl	PSHARED(%rdi), %esi
#if MUTEX != 0
	addq	$MUTEX, %rdi
#endif
	callq	__lll_unlock_wake
#if MUTEX != 0
	subq	$MUTEX, %rdi
#endif
	jmp	7b

	/* Overflow.  */
8:	decl	NR_READERS(%rdi)
	movl	$EAGAIN, %edx
	jmp	9b

	/* Overflow.  */
4:	decl	READERS_QUEUED(%rdi)
	movl	$EAGAIN, %edx
	jmp	9b

10:	movl	PSHARED(%rdi), %esi
#if MUTEX != 0
	addq	$MUTEX, %rdi
#endif
	callq	__lll_unlock_wake
#if MUTEX != 0
	subq	$MUTEX, %rdi
#endif
	jmp	11b

12:	movl	PSHARED(%rdi), %esi
#if MUTEX == 0
	addq	$MUTEX, %rdi
#endif
	callq	__lll_lock_wait
#if MUTEX != 0
	subq	$MUTEX, %rdi
#endif
	jmp	13b
	cfi_endproc
	.size	__pthread_rwlock_rdlock,.-__pthread_rwlock_rdlock

#ifndef __pthread_rwlock_rdlock
strong_alias (__pthread_rwlock_rdlock, pthread_rwlock_rdlock)
hidden_def (__pthread_rwlock_rdlock)
#endif
