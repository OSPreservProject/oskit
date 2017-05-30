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
 * Dump out the boot_info struct nicely.
 */

#include <oskit/x86/pc/base_multiboot.h>
#include <oskit/x86/multiboot.h>
#include <oskit/x86/base_vm.h>
#include <oskit/c/stdio.h>

void
multiboot_info_dump(struct multiboot_info *bi)
{
	struct multiboot_module *m;
	unsigned i;

	printf("MultiBoot Info (flags: 0x%b)\n",
	       bi->flags, MULTIBOOT_FLAGS_FORMAT);

	if (bi->flags & MULTIBOOT_MEMORY)
		printf("  PC Memory: lower %dK, upper %dK\n",
		       bi->mem_lower, bi->mem_upper);

	if (bi->flags & MULTIBOOT_BOOT_DEVICE)
		printf("  Boot device: {%d,%d,%d,%d}\n",
		       bi->boot_device[0],
		       bi->boot_device[1],
		       bi->boot_device[2],
		       bi->boot_device[3]);

	if (bi->flags & MULTIBOOT_CMDLINE)
		printf("  Kernel command line: `%s'\n",
		       (char *)phystokv(bi->cmdline));

	if (bi->flags & MULTIBOOT_MODS) {
		printf("  Boot modules: %d\n", bi->mods_count);
		m = (struct multiboot_module *)phystokv(bi->mods_addr);
		for (i = 0; i < bi->mods_count; i++) {
			printf("  Module %d: %08x-%08x (size %d)\n",
			       i, m[i].mod_start, m[i].mod_end,
			       m[i].mod_end - m[i].mod_start);
			if (m[i].string)
				printf("    String: `%s' at %08x\n",
				       (char*)phystokv(m[i].string),
				       m[i].string);
		}
	}

	if (bi->flags & MULTIBOOT_AOUT_SYMS)
		printf("  Symbol table (a.out): start 0x%08x,"
		       " symtab 0x%08x, strtab 0x%08x\n",
		       bi->syms.a.addr,
		       bi->syms.a.tabsize, bi->syms.a.strsize);

	if (bi->flags & MULTIBOOT_ELF_SHDR)
		/* XXX */
		printf("  Has MULTIBOOT_ELF_SHDR info\n");

	if (bi->flags & MULTIBOOT_MEM_MAP) {
		struct AddrRangeDesc *rdesc;
		unsigned end;

		printf("  Memory Map:\n      addr: 0x%08x,"
		       " count: %d\n", 
		       bi->mmap_addr, bi->mmap_count);

		/* XXX Count is currently the length in bytes of the
		 * entire structure: GRUB and the multiboot spec
		 * actually call it mmap_length. 
		 */
		rdesc = (struct AddrRangeDesc *)bi->mmap_addr;
		end = bi->mmap_addr + bi->mmap_count;

		while (end > (int)rdesc) {
			/* 
			 * I'd rather use .0 precision for the High
			 * entries (since they're guaranteed to be 0
			 * on a PC), but this works too.  */
			printf("      base: 0x");
			if (rdesc->BaseAddrHigh)
				printf("%08lx", rdesc->BaseAddrHigh);
			printf("%08lx length: 0x", rdesc->BaseAddrLow);

			if (rdesc->LengthHigh)
				printf("%08lx", rdesc->LengthHigh);
			printf("%08lx type: ", rdesc->LengthLow);

			switch (rdesc->Type) {
			case 1:
				printf("memory\n");
				break;
			case 2:
				printf("reserved\n");
				break;
			default:
				printf("undefined (%ld)\n", rdesc->Type);
			}

			(char *)rdesc += rdesc->size + 4;
		};
	}
	
	
}
