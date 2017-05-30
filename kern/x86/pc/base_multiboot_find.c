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

#include <string.h>
#include <oskit/x86/multiboot.h>
#include <oskit/x86/base_vm.h>
#include <oskit/x86/pc/base_multiboot.h>

struct multiboot_module *base_multiboot_find(const char *string)
{
	struct multiboot_module *m = (struct multiboot_module*)
		phystokv(boot_info.mods_addr);
	unsigned i;

	if (!(boot_info.flags & MULTIBOOT_MODS))
		return 0;

	for (i = 0; i < boot_info.mods_count; i++)
	{
		if ((m[i].string) &&
		    (strcmp((char*)phystokv(m[i].string), string) == 0))
			return &m[i];
	}

	return 0;
}

