/*
 * Copyright (c) 2000 University of Utah and the Flux Group.
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

#include <oskit/error.h>
#include <oskit/x86/pc/reset.h>
#include <oskit/x86/base_gdt.h>
#include <oskit/x86/base_paging.h>
#include <oskit/x86/base_cpu.h>
#include <oskit/c/stdio.h>
#include <oskit/c/stdlib.h>


extern char **boot_argv;
extern int    boot_argc;

extern void pc_reset_in(void) OSKIT_NORETURN;

static void (*return_address)(void); /* Can't say: OSKIT_NORETURN */

oskit_error_t 
init(void)
{
        int i;
	/* Look for a return address. */
        for (i = 0; i < boot_argc; i++) {
		if (strcmp(boot_argv[i], "-retaddr") == 0
		    && i+1 < boot_argc) {
			return_address = (void (*)(void))strtoul(boot_argv[i+1], 0, 0);
			break;
		}
        }
        return 0;
}

#if 0
void
_exit(int rc)
{
	/* First half depends on having a working console */
	printf("_exit(%d) called; rebooting...\n", rc);
	if (return_address)
		printf("Have return_address 0x%08x\n\n", (unsigned int)return_address);
}
#endif

void pc_reset(void)
{
	/* second half should be the final thing we run */
	if (return_address) {
		/*
		 * The cleanup needs to be done here instead of in the
		 * returned-to code because the return address may not
		 * be accessible with our current paging and segment
		 * state.
		 * The order is important here: paging must be disabled
		 * after we reload the gdt.
		 */
		cli();
		clts();
		phys_mem_va = 0;
		linear_base_va = 0;
		base_gdt_init();
		/* Reload all since we changed linear_base_va. */
		base_cpu_load();
		paging_disable();
		((void (*)(void))return_address)();
	}
        pc_reset_in();
}
