/*
 * Copyright (c) 1996-1998, 2000, 2001 University of Utah and the Flux Group.
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
 * Copy the do_boot stub somewhere and jump into it.
 */

#include <oskit/x86/types.h>		/* oskit_size_t */
#include <oskit/x86/far_ptr.h>		/* far_pointer_32 */
#include <oskit/x86/proc_reg.h>		/* cli/sti */
#include <oskit/x86/base_gdt.h>		/* LINEAR_CS */
#include <oskit/x86/base_vm.h>		/* kvtophys */
#include <oskit/x86/pc/base_multiboot.h> /* multiboot_info_dump */
#include <malloc.h>			/* mustmalloc */
#include <string.h>			/* memcpy */
#include <stdio.h>			/* printf */

#include "boot.h"
#include "debug.h"

extern char do_boot[], do_boot_end[];

void
loadkernel(struct kerninfo *ki)
{
	oskit_addr_t entry, load_addr;
	oskit_size_t stub_size;
	void *stub;
	struct far_pointer_32 ptr;
	oskit_addr_t copy_source;
	oskit_size_t copy_size;

#ifdef DEBUG
	multiboot_info_dump(ki->ki_mbinfo);
#endif

	printf("%s: booting...\n\n", ki->ki_filename);

	cli();

	/*
	 * All of the data structures that are important to the kernel,
	 * are in memory guaranteed not to conflict with the kernel image
	 * or with anything else.
	 * However, the kernel image itself is not in its final position,
	 * because that might be right on top of us,
	 * or on top of anything we allocated before reserving the
	 * kernel image region.
	 * Therefore, we must allocate (non-conflicting) memory for
	 * a small stub to copy the kernel image to its final position
	 * and invoke it.
	 */
	stub_size = do_boot_end - do_boot;
	stub = mustmalloc(stub_size);
	memcpy(stub, do_boot, stub_size);

	ptr.seg = LINEAR_CS;
	ptr.ofs = kvtophys(stub);

	/*
	 * The kernel image source and destination regions may overlap,
	 * so we may have to copy either forwards or backwards.
	 *
	 * XXX the "mr" constraint for "ptr" should be "m" but GCC
	 * says "inconsistent operand constraints."
	 *
	 * Be careful what you put as the values for the registers in
	 * the asm's below.
	 * If you have too many indirections and whatnot then GCC
	 * will apparently run out of registers and won't be able to
	 * set, for example, %ebx to the desired value.
	 * The GCC chokeage looked like this for me:
	 *
	 *	fixed or forbidden register was spilled.
	 *	This may be due to a compiler bug or to impossible asm
	 *
	 * That is why I have load_addr and entry be local vars instead of
	 * just putting the expressions in the asm.
	 */
	copy_source = kvtophys(ki->ki_imgaddr);
	copy_size = kerninfo_imagesize(ki);
	entry = ki->ki_mbhdr.entry;
	load_addr = ki->ki_mbhdr.load_addr;

	dprintf("ki->ki_mbinfo = %u-%u\n",
		kvtophys(ki->ki_mbinfo),
		kvtophys(ki->ki_mbinfo) + sizeof(ki->ki_mbinfo));
	dprintf("stub = %p-%p\n", stub, stub + stub_size);

	if (copy_source > load_addr) {
		asm volatile("
			cld
			ljmp	%0
		" : /* no outputs */
		  : "mr" (ptr),
		    "a" (entry),
		    "S" (copy_source),
		    "D" (load_addr),
		    "c" (copy_size),
		    "b" (kvtophys(ki->ki_mbinfo)),
		    "d" (LINEAR_DS));
	}
	else {
		asm volatile("
			std
			ljmp	%0
		" : /* no outputs */
		  : "mr" (ptr),
		    "a" (entry),
		    "S" (copy_source + copy_size - 1),
		    "D" (load_addr + copy_size - 1),
		    "c" (copy_size),
		    "b" (kvtophys(ki->ki_mbinfo)),
		    "d" (LINEAR_DS));
	}
}
