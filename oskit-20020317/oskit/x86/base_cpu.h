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
 * Functions for setting up and activating
 * a basic x86 protected-mode kernel execution environment.
 */
#ifndef _OSKIT_X86_BASE_CPU_H_
#define _OSKIT_X86_BASE_CPU_H_

#include <oskit/compiler.h>
#include <oskit/x86/cpuid.h>

OSKIT_BEGIN_DECLS
/*
 * CPU information for the processor base_cpu_init() was called on.
 */
extern struct cpu_info base_cpuid;

/*
 * Initialize all the processor data structures defining the base environment:
 * the base_cpuid, the base_idt, the base_gdt, and the base_tss.
 */
extern void base_cpu_init(void);

/*
 * Load all of the above into the processor,
 * properly setting up the protected-mode environment.
 */
extern void base_cpu_load(void);

/*
 * Initialize the FPU.
 */
extern void base_fpu_init(void);
OSKIT_END_DECLS

/*
 * Do both at once.
 */
#define base_cpu_setup() (base_cpu_init(), base_cpu_load())

#endif /* _OSKIT_X86_BASE_CPU_H_ */
