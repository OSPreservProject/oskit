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

#include <oskit/x86/base_trap.h>
#include <oskit/x86/base_gdt.h>
#include <oskit/x86/base_tss.h>
#include <oskit/x86/base_cpu.h>
#include <oskit/x86/pc/base_irq.h>

struct cpu_info base_cpuid;

void base_cpu_init(void)
{
	/* Detect the current processor.  */
	cpuid(&base_cpuid);

	/* Initialize the processor tables.  */
	base_trap_init();
	base_irq_init();
	base_gdt_init();
	base_tss_init();
}

