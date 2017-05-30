/*
 * Copyright (c) 1999, 2000 University of Utah and the Flux Group.
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

/*
 * arm8 support code Copyright (c) 1997 ARM Limited
 * arm8 support code Copyright (c) 1997 Causality Limited
 * Copyright (c) 1997,1998 Mark Brinicombe.
 * Copyright (c) 1997 Causality Limited
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Causality Limited.
 * 4. The name of Causality Limited may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY CAUSALITY LIMITED ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL CAUSALITY LIMITED BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * RiscBSD kernel project
 */

/*
 * Processor registers for i386 and i486.
 */
#ifndef	_OSKIT_ARM32_PROC_REG_H_
#define	_OSKIT_ARM32_PROC_REG_H_

#include <oskit/compiler.h>
#include <oskit/config.h>

/*
 * PSR bits. CPSR and SPSR
 */
#define PSR_I		0x80		/* Interrupt enable/disable */
#define PSR_F		0x40		/* FIQ Interrupt enable/disable */
#define PSR_T		0x20		/* Architecture Version */
#define PSR_CCMASK	0xf0000000	/* Condition code mask */

#define PSR_MODE	0x1f		/* mode mask */
#define PSR_USR32_MODE	0x10
#define PSR_FIQ32_MODE	0x11
#define PSR_IRQ32_MODE	0x12
#define PSR_SVC32_MODE	0x13
#define PSR_ABT32_MODE	0x17
#define PSR_UND32_MODE	0x1b
#define PSR_32_MODE	0x10

/*
 * CPU Control Register
 */
#define CPU_CTRL_M	0x0001
#define CPU_CTRL_A	0x0002
#define CPU_CTRL_C	0x0004
#define CPU_CTRL_W	0x0008
#define CPU_CTRL_P	0x0010
#define CPU_CTRL_D	0x0020
#define CPU_CTRL_L	0x0040
#define CPU_CTRL_B	0x0080
#define CPU_CTRL_S	0x0100
#define CPU_CTRL_R	0x0200
#define CPU_CTRL_F	0x0400

#define trunc_line(x)	(((unsigned) (x)) & ~(32-1))

#ifndef	ASSEMBLER
/*
 * Get/Set the the "current" PSR register (not the banked version).
 */
OSKIT_INLINE unsigned
get_cpsr(void)
{
	unsigned bits;

	asm volatile ("mrs %0,cpsr_all" : "=r" (bits));
	
	return bits;
}

OSKIT_INLINE void
set_cpsr(unsigned bits)
{
	asm volatile ("msr cpsr_all,%0" : : "r" (bits));
}

OSKIT_INLINE void
set_spsr(unsigned bits)
{
	asm volatile ("msr spsr_all,%0" : : "r" (bits));
}

/*
 * Enable/Disable interrupts.
 */
OSKIT_INLINE void
disable_interrupts(void)
{
	unsigned bits = get_cpsr();
	
	set_cpsr(bits | PSR_I);
}

OSKIT_INLINE void
enable_interrupts(void)
{
	unsigned bits = get_cpsr();
	
	set_cpsr(bits & ~PSR_I);
}

/*
 * Return state of the interrupt bit in the PSR.
 */
OSKIT_INLINE int
interrupts_enabled(void)
{
	unsigned bits = get_cpsr();
	
	return ((bits & PSR_I) ? 0 : 1);
}

/*
 * Get CPUID register.
 */
OSKIT_INLINE unsigned
get_cpuid(void)
{
	unsigned bits;
	
	asm volatile ("mrc p15, 0, %0, c0, c0, 0" : "=r" (bits));
	
	return bits;
}

/*
 * Set/Get CPU control register.
 */
OSKIT_INLINE unsigned
get_cpuctrl(void)
{
	unsigned bits;
	
	asm volatile ("mrc p15, 0, %0, c1, c0, 0" : "=r" (bits));
	
	return bits;
}

OSKIT_INLINE void
set_cpuctrl(unsigned bits)
{
	asm volatile ("mcr p15, 0, %0, c1, c0, 0" : : "r" (bits));
	asm volatile ("nop; nop; nop; nop");
}

/*
 * Data Fault Status Register.
 */
OSKIT_INLINE unsigned
get_fsr(void)
{
	unsigned bits;
	
	asm volatile ("mrc p15, 0, %0, c5, c0, 0" : "=r" (bits));
	
	return bits;
}

/*
 * Data Fault Address Register
 */
OSKIT_INLINE unsigned
get_far(void)
{
	unsigned bits;
	
	asm volatile ("mrc p15, 0, %0, c6, c0, 0" : "=r" (bits));
	
	return bits;
}

/*
 * Get/Set Domain Control Register.
 */
OSKIT_INLINE unsigned
get_domctrl(void)
{
	unsigned bits;
	
	asm volatile ("mrc p15, 0, %0, c3, c0, 0" : "=r" (bits));

	return bits;
}

OSKIT_INLINE void
set_domctrl(unsigned bits)
{
	asm volatile ("mcr p15, 0, %0, c3, c0, 0" : : "r" (bits));
}

/*
 * Flush the Instruction and Data cache! Note r0 is written, but ignored.
 *
 * XXX ARCHITECTURE DEPENDENCY!
 */
OSKIT_INLINE void
flush_cache_ID(void)
{
	unsigned bits = 0;

	asm volatile ("mcr p15, 0, %0, c7, c7, 0" : : "r" (bits));
}

/*
 * Flush the Instruction! Note r0 is written, but ignored.
 *
 * XXX ARCHITECTURE DEPENDENCY!
 */
OSKIT_INLINE void
flush_cache_I(void)
{
	unsigned bits = 0;

	asm volatile ("mcr p15, 0, %0, c7, c5, 0" : : "r" (bits));
	asm volatile ("nop; nop; nop; nop;");
}

/*
 * Clean a Data cache entry!
 *
 * XXX ARCHITECTURE DEPENDENCY!
 */
OSKIT_INLINE void
clean_cache_entry_D(void *addr)
{
	unsigned bits = trunc_line((unsigned) addr);

	asm volatile ("mcr p15, 0, %0, c7, c10, 1" : : "r" (bits));
}

/*
 * Flush a Data cache entry!
 *
 * XXX ARCHITECTURE DEPENDENCY!
 */
OSKIT_INLINE void
flush_cache_entry_D(void *addr)
{
	unsigned bits = trunc_line((unsigned) addr);

	asm volatile ("mcr p15, 0, %0, c7, c6, 1" : : "r" (bits));
}

/*
 * Drain the write buffer.
 *
 * XXX ARCHITECTURE DEPENDENCY!
 */
OSKIT_INLINE void
drain_write_buffer(void)
{
	unsigned bits = 0;

	asm volatile ("mcr p15, 0, %0, c7, c10, 4" : : "r" (bits));
}

/*
 * Flush the Instruction and Data TLB! Note r0 is written, but ignored.
 *
 * XXX ARCHITECTURE DEPENDENCY!
 */
OSKIT_INLINE void
flush_tlb_ID(void)
{
	unsigned bits = 0;

	asm volatile ("mcr p15, 0, %0, c8, c7, 0" : : "r" (bits));
}

/*
 * Various register accessors.
 */
#define get_fp() \
    ({ \
	unsigned int _temp__; \
	asm("mov %0,fp" : "=r" (_temp__)); \
	_temp__; \
    })

#define get_sp() \
    ({ \
	unsigned int _temp__; \
	asm("mov %0,sp" : "=r" (_temp__)); \
	_temp__; \
    })
#endif

/*
 * Not sure what to do about a timestamp routine yet.
 */
#define get_tsc()	(0)

#endif	/* _OSKIT_X86_PROC_REG_H_ */
