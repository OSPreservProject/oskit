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
#ifndef _OSKIT_X86_BASE_IDT_H_
#define _OSKIT_X86_BASE_IDT_H_

#include <oskit/compiler.h>
#include <oskit/x86/seg.h>


/* The base environment provides a full-size 256-entry IDT.
   This is needed, for example, under VCPI or Intel MP Standard PCs.  */
#define IDTSZ	256

extern struct x86_gate base_idt[IDTSZ];


/* Note that there is no base_idt_init() function,
   because the IDT is used for both trap and interrupt vectors.
   To initialize the processor trap vectors, call base_trap_init().
   Inititalizing hardware interrupt vectors is platform-specific.  */


OSKIT_BEGIN_DECLS
/* Load the base IDT into the CPU.  */
extern void base_idt_load(void);
OSKIT_END_DECLS


#endif /* _OSKIT_X86_BASE_IDT_H_ */
