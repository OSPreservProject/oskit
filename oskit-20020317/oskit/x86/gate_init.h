/*
 * Copyright (c) 1995-1996, 1998 University of Utah and the Flux Group.
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
 * This file defines a simple, clean method provided by the OS toolkit
 * for initializing x86 gate descriptors:
 * call gates, interrupt gates, trap gates, and task gates.
 * Simply create a table of gate_init_entry structures
 * (or use the corresponding assembly-language macros below),
 * and then call the x86_gate_init() routine.
 */
#ifndef _OSKIT_X86_KERN_GATE_INIT_H_
#define _OSKIT_X86_KERN_GATE_INIT_H_

#ifndef ASSEMBLER

#include <oskit/compiler.h>

/* One entry in the list of gates to initialized.
   Terminate with an entry with a null entrypoint.  */
struct gate_init_entry
{
	unsigned entrypoint;
	unsigned short vector;
	unsigned short type;
};

struct x86_gate;

OSKIT_BEGIN_DECLS
/* Initialize a set of gates in a descriptor table.
   All gates will use the same code segment selector, 'entry_cs'.  */
void gate_init(struct x86_gate *dest, const struct gate_init_entry *src,
	       unsigned entry_cs);
OSKIT_END_DECLS

#else /* ASSEMBLER */

/*
 * We'll be using macros to fill in a table in data hunk 2
 * while writing trap entrypoint routines at the same time.
 * Here's the header that comes before everything else.
 */
#define GATE_INITTAB_BEGIN(name)	\
	.data	2			;\
NON_GPROF_ENTRY(name)			;\
	.text

/*
 * Interrupt descriptor table and code vectors for it.
 */
#define	GATE_ENTRY(n,entry,type) \
	.data	2			;\
	.long	entry			;\
	.word	n			;\
	.word	type			;\
	.text

/*
 * Terminator for the end of the table.
 */
#define GATE_INITTAB_END		\
	.data	2			;\
	.long	0			;\
	.text


#endif /* ASSEMBLER */

#endif /* _OSKIT_X86_KERN_GATE_INIT_H_ */
