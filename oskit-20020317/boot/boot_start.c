/*
 * Copyright (c) 1996, 1998 University of Utah and the Flux Group.
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
 * Copy the kernel somewhere and jump in.
 */

#include <oskit/x86/types.h>		/* oskit_size_t */
#include <oskit/x86/far_ptr.h>		/* far_pointer_32 */
#include <oskit/x86/proc_reg.h>		/* cli/sti */
#include <oskit/x86/base_gdt.h>		/* LINEAR_CS */
#include <oskit/x86/base_vm.h>		/* kvtophys */
#include <oskit/x86/pc/base_multiboot.h> /* multiboot_info_dump */
#include <oskit/c/malloc.h>		/* mustmalloc */
#include <oskit/c/strings.h>		/* bcopy */
#include <oskit/c/stdio.h>		/* printf */

#include "boot.h"			/* boot_info */

extern char do_boot[], do_boot_end[];

void boot_start(struct multiboot_info *bi)
{
	oskit_size_t stub_size;
	void *stub;
	struct far_pointer_32 ptr;
	oskit_addr_t copy_source;
	oskit_size_t copy_size;

#ifdef DEBUG
	multiboot_info_dump(bi);
#endif

	printf("Booting kernel...\n\n");

	cli();

	/* All of the data structures that are important to the kernel,
	   are in memory guaranteed not to conflict with the kernel image
	   or with anything else.
	   However, the kernel image itself is not in its final position,
	   because that might be right on top of us,
	   or on top of anything we allocated before reserving the kernel image region.
	   Therefore, we must allocate (non-conflicting) memory for a small stub
	   to copy the kernel image to its final position and invoke it.  */
	stub_size = do_boot_end - do_boot;
	stub = mustmalloc(stub_size);
	bcopy(do_boot, stub, stub_size);

	ptr.seg = LINEAR_CS;
	ptr.ofs = kvtophys(stub);

	/* The kernel image source and destination regions may overlap,
	   so we may have to copy either forwards or backwards.  */
	copy_source = kvtophys(boot_kern_image);
	copy_size = boot_kern_hdr.load_end_addr - boot_kern_hdr.load_addr;
	if (copy_source > boot_kern_hdr.load_addr)
	{
		asm volatile("
			cld
			ljmp	%0
		" :
		  : "mr" (ptr),		/* XXX r is inappropriate but gcc wants it */
		    "a" (boot_kern_hdr.entry),
		    "S" (copy_source),
		    "D" (boot_kern_hdr.load_addr),
		    "c" (copy_size),
		    "b" (kvtophys(bi)),
		    "d" (LINEAR_DS));
	}
	else
	{
		printf("(copying backwards...)\n"); /* XXX */
		asm volatile("
			std
			ljmp	%0
		" :
		  : "mr" (ptr),		/* XXX r is inappropriate but gcc wants it */
		    "a" (boot_kern_hdr.entry),
		    "S" (copy_source + copy_size - 1),
		    "D" (boot_kern_hdr.load_addr + copy_size - 1),
		    "c" (copy_size),
		    "b" (kvtophys(bi)),
		    "d" (LINEAR_DS));
	}
}
