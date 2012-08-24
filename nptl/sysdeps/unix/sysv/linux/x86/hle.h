#ifndef _HLE_H
#define _HLE_H 1

#ifdef __ASSEMBLER__

.macro XBEGIN target
	.byte 0xc7,0xf8 
	.long \target-1f
1:
.endm

.macro XEND
	.byte 0x0f,0x01,0xd5
.endm

.macro XABORT code
	.byte 0xc6,0xf8,\code
.endm

#else

/* RTM */
#define XABORT(status) asm volatile(".byte 0xc6,0xf8,%P0" :: "i" (status))
#define XBEGIN(label)	\
     asm volatile goto(".byte 0xc7,0xf8 ; .long %l0-1f\n1:" ::: "eax","memory" : label)
#define XEND()	  asm volatile(".byte 0x0f,0x01,0xd5" ::: "memory")
#define XFAIL(label) label: asm volatile("" ::: "eax", "memory")
#define XFAIL_STATUS(label, status) label: asm volatile("" : "=a" (status) :: "memory")

#endif

/* Status bits */
#define XABORT_EXPLICIT_ABORT   (1 << 0)
#define XABORT_RETRY		(1 << 1)
#define XABORT_CONFLICT		(1 << 2)
#define XABORT_CAPACITY		(1 << 3)
#define XABORT_DEBUG		(1 << 4)
#define XABORT_STATUS(x)	(((x) >> 24) & 0xff)

#endif
