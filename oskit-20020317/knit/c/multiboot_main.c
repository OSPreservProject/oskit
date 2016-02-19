/*
 * Copyright (c) 1996, 1998-2000 University of Utah and the Flux Group.
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

#include <oskit/x86/multiboot.h>
#include <oskit/x86/base_vm.h>
#include <oskit/x86/base_cpu.h>
#include <oskit/x86/pc/reset.h>
#include <oskit/x86/pc/base_console.h>
#include <oskit/x86/pc/base_multiboot.h>
#include <oskit/x86/proc_reg.h>
#include <oskit/c/stdlib.h>

struct multiboot_info boot_info;

void
multiboot_main(oskit_addr_t boot_info_pa)
{
	/* Copy the multiboot_info structure into our pre-reserved area.
	   This avoids one loose fragment of memory that has to be avoided.  */
	boot_info = *(struct multiboot_info*)phystokv(boot_info_pa);

	/* Identify the CPU and get the processor tables set up.  */
	base_cpu_setup();

	/* Enable processor interrupts so we can take them when we want to */
	sti();
}

