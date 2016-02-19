/*
 * Copyright (c) 1995, 1998 University of Utah and the Flux Group.
 * All rights reserved.
 * 
 * This file is part of the Flux OSKit.  The OSKit is free software, also known
 * as "open source;" you can redistribute it and/or modify it under the terms
 * of the GNU General Public License (GPL), version 2, as published by the Free
 * Software Foundation (FSF).  To explore alternate licensing terms, contact
 * the University of Utah at csl-dist@cs.utah.edu or +1-801-585-3271.
 * 
 * The OSKit is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GPL for more details.  You should have
 * received a copy of the GPL along with the OSKit; see the file COPYING.  If
 * not, write to the FSF, 59 Temple Place #330, Boston, MA 02111-1307, USA.
 */
#ifndef _OSKIT_X86_DEBUG_REG_H_
#define _OSKIT_X86_DEBUG_REG_H_

/* Bits in DR7 - debug control register */
#define DR7_LEN3	0xc0000000
#define DR7_RW3		0x30000000
#define DR7_LEN2	0x0c000000
#define DR7_RW2		0x03000000
#define DR7_LEN1	0x00c00000
#define DR7_RW1		0x00300000
#define DR7_LEN0	0x000c0000
#define DR7_RW0		0x00030000
#define DR7_GD		0x00002000
#define DR7_GE		0x00000200
#define DR7_LE		0x00000100
#define DR7_G3		0x00000080
#define DR7_L3		0x00000040
#define DR7_G2		0x00000020
#define DR7_L2		0x00000010
#define DR7_G1		0x00000008
#define DR7_L1		0x00000004
#define DR7_G0		0x00000002
#define DR7_L0		0x00000001

/* Shift values for multibit fields in DR7 */
#define DR7_LEN3_SHIFT	30
#define DR7_RW3_SHIFT	28
#define DR7_LEN2_SHIFT	26
#define DR7_RW2_SHIFT	24
#define DR7_LEN1_SHIFT	22
#define DR7_RW1_SHIFT	20
#define DR7_LEN0_SHIFT	18
#define DR7_RW0_SHIFT	16

/* Values for LEN fields in DR7 */
#define DR7_LEN_1	0
#define DR7_LEN_2	1
#define DR7_LEN_4	3

/* Values for RW fields in DR7 */
#define DR7_RW_INST	0	/* Break on instruction execution */
#define DR7_RW_WRITE	1	/* Break on data writes */
#define DR7_RW_IO	2	/* Break on I/O reads and writes (Pentium only) */
#define DR7_RW_DATA	3	/* Break on data reads and writes */


/* Bits in DR6 - debug status register */
#define DR6_BT		0x00008000
#define DR6_BS		0x00004000
#define DR6_BD		0x00002000
#define DR6_B3		0x00000008
#define DR6_B2		0x00000004
#define DR6_B1		0x00000002
#define DR6_B0		0x00000001


#include <oskit/compiler.h>

OSKIT_INLINE unsigned get_dr0(void);
OSKIT_INLINE unsigned get_dr1(void);
OSKIT_INLINE unsigned get_dr2(void);
OSKIT_INLINE unsigned get_dr3(void);
OSKIT_INLINE unsigned get_dr6(void);
OSKIT_INLINE unsigned get_dr7(void);
OSKIT_INLINE void set_dr0(unsigned val);
OSKIT_INLINE void set_dr1(unsigned val);
OSKIT_INLINE void set_dr2(unsigned val);
OSKIT_INLINE void set_dr3(unsigned val);
OSKIT_INLINE void set_dr6(unsigned val);
OSKIT_INLINE void set_dr7(unsigned val);
OSKIT_INLINE void set_b0(unsigned addr, unsigned len, unsigned rw);
OSKIT_INLINE void set_b1(unsigned addr, unsigned len, unsigned rw);
OSKIT_INLINE void set_b2(unsigned addr, unsigned len, unsigned rw);
OSKIT_INLINE void set_b3(unsigned addr, unsigned len, unsigned rw);

/* Functions to set debug registers.  */

OSKIT_INLINE unsigned get_dr0(void)
{
	unsigned val;
	asm volatile("movl %%dr0,%0" : "=r" (val));
	return val;
}

OSKIT_INLINE unsigned get_dr1(void)
{
	unsigned val;
	asm volatile("movl %%dr1,%0" : "=r" (val));
	return val;
}

OSKIT_INLINE unsigned get_dr2(void)
{
	unsigned val;
	asm volatile("movl %%dr2,%0" : "=r" (val));
	return val;
}

OSKIT_INLINE unsigned get_dr3(void)
{
	unsigned val;
	asm volatile("movl %%dr3,%0" : "=r" (val));
	return val;
}

OSKIT_INLINE unsigned get_dr6(void)
{
	unsigned val;
	asm volatile("movl %%dr6,%0" : "=r" (val));
	return val;
}

OSKIT_INLINE unsigned get_dr7(void)
{
	unsigned val;
	asm volatile("movl %%dr7,%0" : "=r" (val));
	return val;
}

OSKIT_INLINE void set_dr0(unsigned val)
{
	asm volatile("movl %0,%%dr0" : : "r" (val));
}


/* Functions to read debug registers.  */

OSKIT_INLINE void set_dr1(unsigned val)
{
	asm volatile("movl %0,%%dr1" : : "r" (val));
}

OSKIT_INLINE void set_dr2(unsigned val)
{
	asm volatile("movl %0,%%dr2" : : "r" (val));
}

OSKIT_INLINE void set_dr3(unsigned val)
{
	asm volatile("movl %0,%%dr3" : : "r" (val));
}

OSKIT_INLINE void set_dr6(unsigned val)
{
	asm volatile("movl %0,%%dr6" : : "r" (val));
}

OSKIT_INLINE void set_dr7(unsigned val)
{
	asm volatile("movl %0,%%dr7" : : "r" (val));
}


/* Functions to set global breakpoints.  */
/* Note that the address must be the LINEAR address */

OSKIT_INLINE void set_b0(unsigned addr, unsigned len, unsigned rw)
{
	set_dr0(addr);
	addr = ((get_dr7() & ~(DR7_LEN0 | DR7_RW0))
	        | (len << DR7_LEN0_SHIFT) | (rw << DR7_RW0_SHIFT)
		| DR7_GE | DR7_G0);
	set_dr7(addr);
}

OSKIT_INLINE void set_b1(unsigned addr, unsigned len, unsigned rw)
{
	set_dr1(addr);
	set_dr7((get_dr7() & ~(DR7_LEN1 | DR7_RW1))
	        | (len << DR7_LEN1_SHIFT) | (rw << DR7_RW1_SHIFT)
		| DR7_GE | DR7_G1);
}

OSKIT_INLINE void set_b2(unsigned addr, unsigned len, unsigned rw)
{
	set_dr2(addr);
	set_dr7((get_dr7() & ~(DR7_LEN2 | DR7_RW2))
	        | (len << DR7_LEN2_SHIFT) | (rw << DR7_RW2_SHIFT)
		| DR7_GE | DR7_G2);
}

OSKIT_INLINE void set_b3(unsigned addr, unsigned len, unsigned rw)
{
	set_dr3(addr);
	set_dr7((get_dr7() & ~(DR7_LEN3 | DR7_RW3))
	        | (len << DR7_LEN3_SHIFT) | (rw << DR7_RW3_SHIFT)
		| DR7_GE | DR7_G3);
}



#endif /* _OSKIT_X86_DEBUG_REG_H_ */
