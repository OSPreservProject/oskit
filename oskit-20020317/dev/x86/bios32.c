/*
 * Copyright (c) 1997-1998 University of Utah and the Flux Group.
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
 * Default implementation of x86-specific fdev functions
 * to access 32-bit BIOS (BIOS32) services such as Plug & Play and PCI.
 */

#include <oskit/page.h>
#include <oskit/error.h>
#include <oskit/x86/types.h>
#include <oskit/x86/far_ptr.h>
#include <oskit/x86/proc_reg.h>
#include <oskit/x86/pc/bios32.h>
#include <oskit/dev/error.h>
#include <oskit/dev/dev.h>

static void *e0000_va;
static struct bios32 *directory;
static void *entry_page_va;

oskit_error_t
oskit_bios32_find_service(unsigned service, oskit_addr_t *out_start,
			 oskit_addr_t *out_length, oskit_addr_t *out_entry)
{
	oskit_error_t rc;

	/* Map the BIOS memory area from E0000 to FFFFF */
	if (e0000_va == 0) {
		rc = osenv_mem_map_phys(0xe0000, 0x20000, &e0000_va, 0);
		if (rc)
			return rc;
	}

	/* Find the BIOS32 service directory structure */
	if (directory == 0) {
		struct bios32 *dir;
		for (dir = e0000_va; ; dir++) {
			unsigned char sum;
			int i;

			if (dir == e0000_va + 0x20000)
				return OSKIT_E_DEV_NO_BIOS32;
			if (dir->signature != BIOS32_SIGNATURE)
				continue;

			/* Make sure the checksum adds up */
			sum = 0;
			for (i = dir->length * 16 - 1; i >= 0; i--)
				sum += ((unsigned char*)dir)[i];
			if (sum != 0)
				continue;

			/* Make sure this is the revision we want */
			if (dir->revision != 0) {
				osenv_log(OSENV_LOG_WARNING,
					"BIOS32 Service Directory structure "
					"with unsupported revision %d at %08x",
					dir->revision,
					(void*)dir - e0000_va + 0xe0000);
				continue;
			}

			/* Found it! */
			directory = dir;
			break;
		}
	}

	/*
	 * Map two physical pages into our address space
	 * starting with the one containing the entrypoint,
	 * as specified in the BIOS32 specification.
	 * Note that the entrypoint itself isn't necessarily
	 * in the e0000-ffff0 range that the directory struct is in.
	 */
	if (entry_page_va == 0) {
		oskit_addr_t entry_page_pa = directory->entry & ~PAGE_MASK;

		rc = osenv_mem_map_phys(entry_page_pa, PAGE_SIZE * 2,
				       &entry_page_va, 0);
		if (rc)
			return rc;
	}

	/*
	 * Call the BIOS32 service directory entrypoint
	 */
	{
		struct far_pointer_32 call_vector;
		unsigned char retcode;

		call_vector.seg = get_cs();
		call_vector.ofs = (oskit_addr_t)entry_page_va +
				  (directory->entry & PAGE_MASK);
		asm volatile("lcall (%6)"
			     : "=a" (retcode),
			       "=b" (*out_start),
			       "=c" (*out_length),
			       "=d" (*out_entry)
			     : "a" (service),
			       "b" (0),
			       "r" (&call_vector));
		if (retcode != 0) {
			if (retcode == 0x80)
				return OSKIT_E_DEV_NO_BIOS32_SERVICE;
			return OSKIT_E_UNEXPECTED;
		}
		return 0;
	}
}
