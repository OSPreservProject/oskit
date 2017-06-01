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
/*
 * Definitions for a basic x86 32-bit Task State Segment (TSS)
 * which is set up and and used by the base environment as a "default" TSS.
 */
#ifndef _OSKIT_X86_BASE_TSS_H_
#define _OSKIT_X86_BASE_TSS_H_

#include <oskit/compiler.h>
#include <oskit/x86/tss.h>

extern struct x86_tss base_tss;

OSKIT_BEGIN_DECLS
/* Initialize the base TSS with sensible default values.
   The current stack pointer is used as the TSS's ring 0 stack pointer.  */
extern void base_tss_init(void);

/* Load the base TSS into the current CPU.
   Assumes that the base GDT is already loaded,
   and that the BASE_TSS descriptor in the GDT refers to the base TSS.  */
extern void base_tss_load(void);
OSKIT_END_DECLS

#endif /* _OSKIT_X86_BASE_TSS_H_ */
