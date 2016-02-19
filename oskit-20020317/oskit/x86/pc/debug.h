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
#ifndef _KUKM_I386_PC_DEBUG_H_
#define _KUKM_I386_PC_DEBUG_H_

#ifdef ASSEMBLER
#ifdef DEBUG


/* Poke a character directly onto the VGA text display,
   as a very quick, mostly-reliable status indicator.
   Assumes ss is a kernel data segment register.  */
#define POKE_STATUS(char,scratch)		\
	ss/*XXX gas bug */			;\
	movl	%ss:_phys_mem_va,scratch	;\
	addl	$0xb8000+80*2*13+40*2,scratch	;\
	movb	char,%ss:(scratch)		;\
	movb	$0xf0,%ss:1(scratch)


#else !DEBUG

#define POKE_STATUS(char,scratch)

#endif !DEBUG
#else !ASSEMBLER
#ifdef DEBUG

#include <mach/machine/vm_types.h>


#define POKE_STATUS(string)						\
	({	unsigned char *s = (string);				\
		extern oskit_addr_t phys_mem_va;				\
		short *d = (short*)(phys_mem_va+0xb8000+80*2*13+40*2);	\
		while (*s) { (*d++) = 0x3000 | (*s++); }		\
		*d = ' ';						\
	})


#else !DEBUG

#define POKE_STATUS(char)

#endif !DEBUG
#endif !ASSEMBLER


#include_next "debug.h"

#endif _KUKM_I386_PC_DEBUG_H_
