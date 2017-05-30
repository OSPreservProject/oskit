/*
 * Copyright (c) 1999 University of Utah and the Flux Group.
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

#include <oskit/arm32/shark/base_console.h>
#include <oskit/arm32/shark/base_vm.h>
#include <oskit/arm32/shark/base_multiboot.h>
#include <oskit/arm32/base_trap.h>
#include <oskit/arm32/base_cpu.h>
#include <oskit/arm32/ofw/ofw.h>
#include <oskit/arm32/ofw/ofw_console.h>
#include <oskit/arm32/proc_reg.h>
#include <oskit/c/stdlib.h>

struct multiboot_info boot_info;

extern int main(int argc, char *argv[], char *envp[]);
extern char **environ;

#ifdef __ELF__
extern void __oskit_init(void);
extern void __oskit_fini(void);
#endif

void
multiboot_main(void *ofw_handle, oskit_addr_t boot_info_pa)
{
	int argc = 0;
	char **argv = 0;

	/* Copy the multiboot_info structure into our pre-reserved area.
	   This avoids one loose fragment of memory that has to be avoided.  */
	boot_info = *(struct multiboot_info*)phystokv(boot_info_pa);

	OF_init(ofw_handle);
	ofw_base_console_init();
#if 0
	kprintf("MM: %p %p\n", ofw_handle, boot_info_pa);

	ofw_getvirtualtranslations();
#endif	
	/* Identify the CPU and get the processor tables set up.  */
	base_cpu_setup();

	/* Initialize the memory allocator and find all available memory.  */
	base_multiboot_init_mem();

	/* Parse the command line into nice POSIX-like args and environment.  */
	base_multiboot_init_cmdline(&argc, &argv);

#ifdef  NETBOOT_WORKS
	/* Look for a return address. */
	for (i = 0; i < oskit_bootargc; i++)
		if (strcmp(oskit_bootargv[i], "-retaddr") == 0
		    && i+1 < oskit_bootargc) {
			return_address = strtoul(oskit_bootargv[i+1], 0, 0);
			break;
		}
#endif
	/* Initialize the console */
	base_console_init(oskit_bootargc, oskit_bootargv);

#ifdef __ELF__
	/* Make sure deinit code gets called on exit. */
	atexit(__oskit_fini);

	/* Call init code. */
	__oskit_init();
#endif

	/* Invoke the main program. */
	exit(main(argc, argv, environ));
}
