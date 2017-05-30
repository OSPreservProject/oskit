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
/*
 * This overrides the multiboot_main from libkern.
 * We need to remove the -d from our bootargs so the debugger isn't
 * turned on us -- we want to just pass it to what we boot.
 */

#include <oskit/x86/multiboot.h>
#include <oskit/x86/base_vm.h>
#include <oskit/x86/base_cpu.h>
#include <oskit/x86/proc_reg.h>
#include <oskit/x86/pc/reset.h>
#include <oskit/x86/pc/base_console.h>
#include <oskit/x86/pc/base_multiboot.h>
#include <oskit/c/stdlib.h>
#include <oskit/c/string.h>

struct multiboot_info boot_info;

extern oskit_addr_t return_address;

extern int main(int argc, char *argv[], char *envp[]);
extern char **environ;
extern char *enable_debug_arg;	/* main.c */

#ifdef __ELF__
extern void __oskit_init(void);
extern void __oskit_fini(void);
#endif

#ifdef GPROF
extern int enable_gprof;
extern void base_gprof_init();
#endif

void
multiboot_main(oskit_addr_t boot_info_pa)
{
	int argc = 0;
	char **argv = 0;
	int i;
	char **ep;

	/* Copy the multiboot_info structure into our pre-reserved area.
	   This avoids one loose fragment of memory that has to be avoided.  */
	boot_info = *(struct multiboot_info*)phystokv(boot_info_pa);

	/* Identify the CPU and get the processor tables set up.  */
	base_cpu_setup();

	/* Initialize the memory allocator and find all available memory.  */
	base_multiboot_init_mem();

	/* Parse the command line into nice POSIX-like args and environment.  */
	base_multiboot_init_cmdline(&argc, &argv);

	/* Remove the -d flag because we don't want to be debugged, we
           want to debug our kids. */
	for (i = 0; i < oskit_bootargc; i++)
		if (strcmp(oskit_bootargv[i], "-d") == 0) {
			int j;

			enable_debug_arg = oskit_bootargv[i];
			for (j = i; j < oskit_bootargc; j++)
				oskit_bootargv[j] = oskit_bootargv[j + 1];
			oskit_bootargc--;
			break;
		}
	/* Likewise for a "GDB_COM" environment variable.  */
	for (ep = environ; *ep; ++ep)
		if (strncmp(*ep, "GDB_COM=", 8) == 0) {
			enable_debug_arg = *ep;
			i = 0;
			do
				ep[i] = ep[i + 1];
			while (ep[i++]);
			break;
		}


	/* Look for a return address. */
	for (i = 0; i < oskit_bootargc; i++)
		if (strcmp(oskit_bootargv[i], "-retaddr") == 0
		    && i+1 < oskit_bootargc) {
			return_address = strtoul(oskit_bootargv[i+1], 0, 0);
			break;
		}

	/* Enable interrupts, since we may be using remote gdb. */
	sti();

	/* Initialize the console */
	base_console_init(oskit_bootargc, oskit_bootargv);

#ifdef GPROF
	if (enable_gprof)
		base_gprof_init();
#endif

#ifdef __ELF__
	/* Make sure deinit code gets called on exit. */
	atexit(__oskit_fini);

	/* Call init code. */
	__oskit_init();
#endif

	/* Invoke the main program. */
	exit(main(argc, argv, environ));
}
