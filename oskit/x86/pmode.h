/*
 * Copyright (c) 1994-1996, 1998 University of Utah and the Flux Group.
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
#ifndef _OSKIT_X86_PMODE_H_
#define _OSKIT_X86_PMODE_H_

#include <oskit/compiler.h>
#include <oskit/x86/i16.h>
#include <oskit/x86/proc_reg.h>

/* Enter protected mode on x86 machines.
   Assumes:
	* Running in real mode.
	* Interrupts are turned off.
	* A20 is enabled (if on a PC).
	* A suitable GDT is already loaded.

   You must supply a 16-bit code segment
   equivalent to the real-mode code segment currently in use.

   You must reload all segment registers except CS
   immediately after invoking this macro.

   XXX THESE INLINE FUNCTIONS ARE DANGEROUS!! XXX
   Not because of what they do, but because of their structure.
   They contain ASM labels, so if they are inlined more than once
   in a function, you wind up with a multiply defined symbols.
   This can happen, for example, if GCC is doing aggressive inlining (-O3).
   I avoided this problem once already in x86/pc/i16_raw.c by rearranging
   the functions and counting on a particular heuristic of GCC (that it
   will not inline calls that precede a function's definition).

*/
OSKIT_INLINE void i16_enter_pmode(int prot_cs)
{
	extern unsigned short prot_jmp[];

	/* Switch to protected mode.  */
	prot_jmp[3] = prot_cs;
	asm volatile("
		movl	%0,%%cr0
	prot_jmp:
	_prot_jmp:
		ljmp	$0,$1f
	1:
	" : : "r" (i16_get_cr0() | CR0_PE));
}



/* Leave protected mode and return to real mode.
   Assumes:
	* Running in protected mode
	* Interrupts are turned off.
	* Paging is turned off.
	* All currently loaded segment registers
	  contain 16-bit segments with limits of 0xffff.

   You must supply a real-mode code segment
   equivalent to the protected-mode code segment currently in use.

   You must reload all segment registers except CS
   immediately after invoking this function.
*/
OSKIT_INLINE void i16_leave_pmode(int real_cs)
{
	extern unsigned short real_jmp[];

	/*
	 * Switch back to real mode.
	 * Note: switching to the real-mode code segment
	 * _must_ be done with an _immediate_ far jump,
	 * not an indirect far jump.  At least on my Am386DX/40,
	 * an indirect far jump leaves the code segment read-only.
	 */
	real_jmp[3] = real_cs;
	asm volatile("
		movl	%0,%%cr0
		jmp	1f
	1:
	real_jmp:
	_real_jmp:
		ljmp	$0,$1f
	1:
	" : : "r" (i16_get_cr0() & ~CR0_PE));
}



#endif /* _OSKIT_X86_PMODE_H_ */
