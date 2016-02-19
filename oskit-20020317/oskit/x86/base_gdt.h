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
/*
 * This file defines a basic global descriptor table that can be used
 * for simple protected-mode execution in both kernel and user mode.
 */
#ifndef _OSKIT_X86_BASE_GDT_H_
#define _OSKIT_X86_BASE_GDT_H_

#include <oskit/x86/seg.h>


#define BASE_TSS	0x08
#define KERNEL_CS	0x10	/* Kernel's PL0 code segment */
#define KERNEL_DS	0x18	/* Kernel's PL0 data segment */
#define KERNEL_16_CS	0x20	/* 16-bit version of KERNEL_CS */
#define KERNEL_16_DS	0x28	/* 16-bit version of KERNEL_DS */
#define LINEAR_CS	0x30	/* PL0 code segment mapping to linear space */
#define LINEAR_DS	0x38	/* PL0 data segment mapping to linear space */
#define USER_CS		0x43	/* User-defined descriptor, RPL=3 */
#define USER_DS		0x4b	/* User-defined descriptor, RPL=3 */
#define KERNEL_TRAP_TSS 0x50	/* Used by stack-overflow detection code */

/* Leave a little additional room beyond this for customization */
#define GDTSZ		(0x80/8)


#ifndef ASSEMBLER

#include <oskit/compiler.h>

extern struct x86_desc base_gdt[GDTSZ];

OSKIT_BEGIN_DECLS
/* Initialize the base GDT descriptors with sensible defaults.  */
extern void base_gdt_init(void);

/* Load the base GDT into the CPU.  */
extern void base_gdt_load(void);

/* 16-bit versions of the above functions,
   which can be executed in real mode.  */
extern void i16_base_gdt_init(void);
extern void i16_base_gdt_load(void);
OSKIT_END_DECLS

#endif /* not ASSEMBLER */

#endif /* _OSKIT_X86_BASE_GDT_H_ */
