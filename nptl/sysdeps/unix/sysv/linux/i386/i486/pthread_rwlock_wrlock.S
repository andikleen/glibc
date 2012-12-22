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

#include <sysdep.h>
#include <lowlevellock.h>
#include <lowlevelrwlock.h>
#include <pthread-errnos.h>
#include <kernel-features.h>
#include <hle.h>

#include <stap-probe.h>

#ifdef PIC
#define MO(x) x@GOTOFF(%edx)
#else
#define MO(x) x
#endif

	.text

	.globl	__pthread_rwlock_wrlock
	.type	__pthread_rwlock_wrlock,@function
	.align	16
__pthread_rwlock_wrlock:
	cfi_startproc
	pushl	%esi
	cfi_adjust_cfa_offset(4)
	pushl	%ebx
	cfi_adjust_cfa_offset(4)
	cfi_offset(%esi, -8)
	cfi_offset(%ebx, -12)

	xorl	%esi, %esi
	movl	12(%esp), %ebx

	LIBC_PROBE (wrlock_entry, 1, %ebx)

#ifdef PIC
	SETUP_PIC_REG(dx)
	addl $_GLOBAL_OFFSET_TABLE_,%edx
#endif

	cmpl  $0,MO(__elision_available)
	jz    not_elided_wrlock	

	cmpb  $0,ELIDING(%ebx)
	js    not_elided_wrlock
	jnz   try_trans_wrlock
	/* zero: use default */

	cmpl    $0,MO(__rwlock_rtm_enabled)
	jz      not_elided_wrlock

try_trans_wrlock:
	XBEGIN abort_wrlock

	/* Lock writer free? */
	/* Ignore readers because we don't need them */
	cmpl  $0,WRITER(%ebx)
	jnz   1f
	cmpl  $0,NR_READERS(%ebx)
	jnz   1f

	/* Lock is free. Run with transaction */
	xor   %eax,%eax

	pop   %ebx
	cfi_adjust_cfa_offset(-4)
	pop   %esi
	cfi_adjust_cfa_offset(-4)
	ret

	/* Lock is not free. Run */
1:	XABORT 0xff
	jmp   not_elided_wrlock

	/* Abort happened. */
abort_wrlock:
	cmpl  $0,MO(__tsx_abort_hook)
	jz    1f
	push  %eax
	cfi_adjust_cfa_offset(4)
	call  *MO(__tsx_abort_hook)
	pop   %eax
	cfi_adjust_cfa_offset(-4)
	xorl	%esi, %esi	/* needed? */
	movl	12(%esp), %ebx
#ifdef PIC
	SETUP_PIC_REG(dx)
	addl $_GLOBAL_OFFSET_TABLE_,%edx
#endif

1:

	/* Otherwise we just fall back directly to the lock.
	   Here's the place to add more adaptation. */

not_elided_wrlock:

	/* Get the lock.  */
	movl	$1, %edx
	xorl	%eax, %eax
	LOCK
#if MUTEX == 0
	cmpxchgl %edx, (%ebx)
#else
	cmpxchgl %edx, MUTEX(%ebx)
#endif
	jnz	1f

2:	movl	WRITER(%ebx), %eax
	testl	%eax, %eax
	jne	14f
	cmpl	$0, NR_READERS(%ebx)
	je	5f

3:	addl	$1, WRITERS_QUEUED(%ebx)
	je	4f

	movl	WRITERS_WAKEUP(%ebx), %edx

	LOCK
#if MUTEX == 0
	subl	$1, (%ebx)
#else
	subl	$1, MUTEX(%ebx)
#endif
	jne	10f

11:
#ifdef __ASSUME_PRIVATE_FUTEX
	movzbl	PSHARED(%ebx), %ecx
	xorl	$FUTEX_PRIVATE_FLAG|FUTEX_WAIT, %ecx
#else
	movzbl	PSHARED(%ebx), %ecx
# if FUTEX_WAIT != 0
	orl	$FUTEX_WAIT, %ecx
# endif
	xorl	%gs:PRIVATE_FUTEX, %ecx
#endif
	addl	$WRITERS_WAKEUP, %ebx
	movl	$SYS_futex, %eax
	ENTER_KERNEL

	subl	$WRITERS_WAKEUP, %ebx

	/* Reget the lock.  */
	movl	$1, %edx
	xorl	%eax, %eax
	LOCK
#if MUTEX == 0
	cmpxchgl %edx, (%ebx)
#else
	cmpxchgl %edx, MUTEX(%ebx)
#endif
	jnz	12f

13:	subl	$1, WRITERS_QUEUED(%ebx)
	jmp	2b

5:	xorl	%edx, %edx
	movl	%gs:TID, %eax
	movl	%eax, WRITER(%ebx)
9:	LOCK
#if MUTEX == 0
	subl	$1, (%ebx)
#else
	subl	$1, MUTEX(%ebx)
#endif
	jne	6f
7:

	movl	%edx, %eax
	popl	%ebx
	cfi_adjust_cfa_offset(-4)
	cfi_restore(%ebx)
	popl	%esi
	cfi_adjust_cfa_offset(-4)
	cfi_restore(%esi)
	ret

	cfi_adjust_cfa_offset(8)
	cfi_offset(%esi, -8)
	cfi_offset(%ebx, -12)
1:
#if MUTEX == 0
	movl	%ebx, %edx
#else
	leal	MUTEX(%ebx), %edx
#endif
	movzbl	PSHARED(%ebx), %ecx
	call	__lll_lock_wait
	jmp	2b

14:	cmpl	%gs:TID	, %eax
	jne	3b
	movl	$EDEADLK, %edx
	jmp	9b

6:
#if MUTEX == 0
	movl	%ebx, %eax
#else
	leal	MUTEX(%ebx), %eax
#endif
	movzbl	PSHARED(%ebx), %ecx
	call	__lll_unlock_wake
	jmp	7b

4:	subl	$1, WRITERS_QUEUED(%ebx)
	movl	$EAGAIN, %edx
	jmp	9b

10:
#if MUTEX == 0
	movl	%ebx, %eax
#else
	leal	MUTEX(%ebx), %eax
#endif
	movzbl	PSHARED(%ebx), %ecx
	call	__lll_unlock_wake
	jmp	11b

12:
#if MUTEX == 0
	movl	%ebx, %edx
#else
	leal	MUTEX(%ebx), %edx
#endif
	movzbl	PSHARED(%ebx), %ecx
	call	__lll_lock_wait
	jmp	13b
	cfi_endproc
	.size	__pthread_rwlock_wrlock,.-__pthread_rwlock_wrlock

#ifndef __pthread_rwlock_wrlock
strong_alias (__pthread_rwlock_wrlock, pthread_rwlock_wrlock)
hidden_def (__pthread_rwlock_wrlock)
#endif
