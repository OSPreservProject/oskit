/*
 * Copyright (c) 1996, 1998, 1999 University of Utah and the Flux Group.
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
#include <oskit/lmm.h>
#include <oskit/arm32/multiboot.h>
#include <oskit/arm32/physmem.h>
#include <oskit/arm32/shark/base_vm.h>
#include <oskit/arm32/shark/phys_lmm.h>
#include <oskit/arm32/shark/base_multiboot.h>

#if 0
#define DPRINTF(fmt, args... ) kprintf(__FUNCTION__  ": " fmt , ## args)
#else
#define DPRINTF(fmt, args... )
#endif

#define skip(hole_min, hole_max)					\
	if ((max > (hole_min)) && (min < (hole_max)))			\
	{								\
		if (min < (hole_min)) max = (hole_min);			\
		else { min = (hole_max); goto retry; }			\
	}

void base_multiboot_init_mem(void)
{
	oskit_addr_t min, max;
	extern char _start[], end[];
	struct multiboot_memmap *pmap;
	int i, j;

	/* Memory regions to skip.  */
	oskit_addr_t cmdline_start_pa = boot_info.flags & MULTIBOOT_CMDLINE
		? boot_info.cmdline : 0;
	oskit_addr_t cmdline_end_pa = cmdline_start_pa
		? cmdline_start_pa+strlen((char*)phystokv(cmdline_start_pa))+1
		: 0;

	/* Initialize the base memory allocator
	   according to the PC's physical memory regions.  */
	phys_lmm_init();

	/*
	 * Add to the free list all of the memory in the memory map
	 * descriptors, carefully avoiding areas occupied by boot information,
	 * as well as our own executable code, data, and bss.
	 */
	pmap = (struct multiboot_memmap *) phystokv(boot_info.mmap_addr);
	for (i = 0; i < boot_info.mmap_count; i++, pmap++) {
	    min = pmap->mem_base;

	    do
	    {
		max = pmap->mem_base + pmap->mem_size;
		
		/* Skip our own text, data, and bss.  */
		skip(kvtophys(_start), kvtophys(end));

		/* Skip the memmap descriptors */
		skip(boot_info.mmap_addr,
		     boot_info.mmap_addr +
		     boot_info.mmap_count * sizeof(*pmap));

		/* Skip the command line argument string */
		skip(cmdline_start_pa, cmdline_end_pa);

		/* Skip the boot modules */
		if ((boot_info.flags & MULTIBOOT_MODS)
		    && (boot_info.mods_count > 0)) {
		    struct multiboot_module *m =
			(struct multiboot_module *)
			    phystokv(boot_info.mods_addr);

		    skip(boot_info.mods_addr,
			 boot_info.mods_addr +
			 boot_info.mods_count*sizeof(*m));
		    
		    for (j = 0; j < boot_info.mods_count; j++) {
			if (m[j].string != 0) {
			    char *s = (char*)phystokv(m[j].string);
			    unsigned len = strlen(s);
			    skip(m[j].string, m[j].string+len+1);
			}
			skip(m[j].mod_start, m[j].mod_end);
		    }
		}

		/* We actually found a contiguous memory block
		   that doesn't conflict with anything else!  Whew!
		   Add it to the free list.  */
		DPRINTF("Adding 0x%x -> 0x%x\n", min, min + (max - min));
		phys_lmm_add(min, max - min);

		/* Continue searching just past the end of this region.  */
		min = max;

		    /* The skip() macro jumps to this label
		       to restart with a different (higher) min address.  */
		retry:
		DPRINTF("Again: 0x%x 0x%x\n", min, max);
	    }
	    while (min < (pmap->mem_base + pmap->mem_size));
	}
	DPRINTF("phys_mem_min:0x%x phys_mem_max:0x%x phys_mem_avail:0x%x\n",
		phys_mem_min, phys_mem_max, lmm_avail(&malloc_lmm, 0));
}

