/*
 * Copyright (c) 1996-1998, 2001 University of Utah and the Flux Group.
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
#include <oskit/x86/base_gdt.h>		/* KERNEL_CS */
#include <oskit/x86/base_vm.h>		/* kvtolin */
#include <oskit/x86/proc_reg.h>		/* set_gdt */
#include <oskit/x86/seg.h>		/* x86_desc */
#include <oskit/x86/types.h>		/* oskit_size_t */

#include <malloc.h>		/* mustmalloc */
#include <string.h>		/* memcpy */

#include "gdt.h"
#include "debug.h"

/*
 * Load the gdt from gdtp, which is len bytes long.
 */
static void
gdt_load(struct x86_desc *gdtp, oskit_size_t len)
{
	struct pseudo_descriptor pdesc;

	/* Create a pseudo-descriptor describing the GDT.  */
	pdesc.limit = len - 1;
	pdesc.linear_base = kvtolin(gdtp);

	/* Load it into the CPU.  */
	set_gdt(&pdesc);

	/*
	 * Reload all the segment registers from the new GDT.
	 */
	asm volatile("
		ljmp	%0,$1f
	1:
	" : : "i" (KERNEL_CS));
	set_ds(KERNEL_DS);
	set_es(KERNEL_DS);
	set_ss(KERNEL_DS);

	/*
	 * Clear out the FS and GS registers by default,
	 * since they're not needed for normal execution of GCC code.
	 */
	set_fs(0);
	set_gs(0);
}

/*
 * Copy the base_gdt off somewhere so when we copy the kernel to its
 * final resting place we don't clobber it.
 *
 * XXX the base_gdt has the addr of base_tss in it.
 */
void
copy_base_gdt(void)
{
	struct x86_desc *gdtp;

	gdtp = (struct x86_desc *)mustmalloc(sizeof(base_gdt));
	memcpy(gdtp, base_gdt, sizeof(base_gdt));
	gdt_load(gdtp, sizeof(base_gdt));
	dprintf("Moved gdt from %p to %p\n", base_gdt, gdtp);
}
