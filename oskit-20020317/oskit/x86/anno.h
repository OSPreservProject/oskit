/*
 * Copyright (c) 1994-1995, 1998 University of Utah and the Flux Group.
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
#ifndef _OSKIT_X86_ANNO_H_
#define _OSKIT_X86_ANNO_H_

#ifndef ASSEMBLER



#else /* ASSEMBLER */


/* Create an arbitrary annotation entry.
   Must switch back to an appropriate segment afterward.  */
#define ANNO_ENTRY(table, val1, val2, val3)	\
	.section .anno,"aw",@progbits		;\
	.long	val1,val2,val3,table

/* Create an annotation entry for code in a text segment.  */
#define ANNO_TEXT(table, val2, val3)		\
9:	ANNO_ENTRY(table, 9b, val2, val3)	;\
	.text



/* The following are for common annotation tables.
   These don't have to be used in any given kernel,
   and others can be defined as convenient.  */


/* The anno_intr table is generally accessed
   on hardware interrupts that occur while running in kernel mode.
   The value is a routine for the trap handler in interrupt.S
   to jump to before processing the hardware interrupt.
   This routine applies to all code from this address
   up to but not including the address of the next ANNO_INTR.
   To disable interrupt redirection for a piece of code,
   place an ANNO_INTR(0) before it.  */

#define ANNO_INTR(routine, arg) \
	ANNO_TEXT(anno_intr, routine, arg)


/* The anno_trap table is accessed
   on processor traps that occur in kernel mode.
   If a match is found in this table,
   the specified alternate handler is run instead of the generic handler.
   A match is found only if the EIP exactly matches an ANNO_TRAP entry
   (i.e. these entries apply to individual instructions, not groups),
   and if the trap type that occurred matches the type specified.  */

#define ANNO_TRAP(routine, arg) \
	ANNO_TEXT(anno_trap, routine, arg)


#endif /* ASSEMBLER */

#endif /* _OSKIT_X86_ANNO_H_ */
