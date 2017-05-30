/*
 * Copyright (c) 1996, 1998, 1999, 2000 University of Utah and the Flux Group.
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

#include <string.h>
#include <oskit/x86/multiboot.h>
#include <oskit/x86/base_vm.h>
#include <oskit/x86/pc/phys_lmm.h>
#include <oskit/x86/pc/base_multiboot.h>

#define skip(hole_min, hole_max)					\
	if ((max > (hole_min)) && (min < (hole_max)))			\
	{								\
		if (min < (hole_min)) max = (hole_min);			\
		else { min = (hole_max); goto retry; }			\
	}

void base_multiboot_init_mem(void)
{
	oskit_addr_t min;
	extern char _start_of_kernel[], end[];

	/* Memory regions to skip.  */
	oskit_addr_t cmdline_start_pa = boot_info.flags & MULTIBOOT_CMDLINE
		? boot_info.cmdline : 0;
	oskit_addr_t cmdline_end_pa = cmdline_start_pa
		? cmdline_start_pa+strlen((char*)phystokv(cmdline_start_pa))+1
		: 0;

#ifndef KNIT
	/* Initialize the base memory allocator
	   according to the PC's physical memory regions.  */
	phys_lmm_init();
#endif

	/* Add to the free list all the memory the boot loader told us about,
	   carefully avoiding the areas occupied by boot information.
	   as well as our own executable code, data, and bss.
	   Start at the end of the BIOS data area.  */
	min = 0x500;
	do
	{
		oskit_addr_t max = 0xffffffff;

		/* Skip the I/O and ROM area.  */
		skip(boot_info.mem_lower * 1024, 0x100000);

		/* Stop at the end of upper memory.  */
		skip(0x100000 + boot_info.mem_upper * 1024, 0xffffffff);

		/* Skip our own text, data, and bss.  */
		skip(kvtophys(_start_of_kernel), kvtophys(end));

		/* Skip the important stuff the bootloader passed to us.  */
		skip(cmdline_start_pa, cmdline_end_pa);
		if ((boot_info.flags & MULTIBOOT_MODS)
		    && (boot_info.mods_count > 0))
		{
			struct multiboot_module *m = (struct multiboot_module*)
				phystokv(boot_info.mods_addr);
			unsigned i;

			skip(boot_info.mods_addr,
			     boot_info.mods_addr +
			     boot_info.mods_count * sizeof(*m));
			for (i = 0; i < boot_info.mods_count; i++)
			{
				if (m[i].string != 0)
				{
					char *s = (char*)phystokv(m[i].string);
					unsigned len = strlen(s);
					skip(m[i].string, m[i].string+len+1);
				}
				skip(m[i].mod_start, m[i].mod_end);
			}
		}

		/* We actually found a contiguous memory block
		   that doesn't conflict with anything else!  Whew!
		   Add it to the free list.  */
		phys_lmm_add(min, max - min);

		/* Continue searching just past the end of this region.  */
		min = max;

		/* The skip() macro jumps to this label
		   to restart with a different (higher) min address.  */
		retry:
	}
	while (min < 0xffffffff);
}

