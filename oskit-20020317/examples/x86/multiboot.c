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
 * Example mini-kernel that boots from a MultiBoot boot loader
 * and prints out the multiboot info and info about the cpu.
 */

#include <oskit/x86/pc/base_multiboot.h>
#include <oskit/x86/base_cpu.h>
#include <oskit/clientos.h>
#include <stdio.h>

/*
 * The MultiBoot startup code will call this function
 * after setting up the base environment.
 * The kernel command line string passed by the boot loader
 * will have been parsed into separate argv option strings,
 * and options of the form FOO=BAR will be separated out
 * into environment variables,
 * which can be accessed using getenv() as in ordinary programs.
 */
int main(int argc, char **argv)
{
	extern char **environ;
	unsigned i;

	oskit_clientos_init();

	printf("\nI was given this command line, environment, and bootopts:\n");
	for (i = 0; i < argc; i++)
		printf("  argv[%d]: `%s'\n", i, argv[i]);
	for (i = 0; environ[i]; i++)
		printf("  environ[%d]: `%s'\n", i, environ[i]);
	for (i = 0; i < oskit_bootargc; i++)
		printf("  oskit_bootargv[%d]: `%s'\n", i, oskit_bootargv[i]);

	multiboot_info_dump(&boot_info);

	printf("\nI'm running on a...\n");
	cpu_info_dump(&base_cpuid);

	return 0;
}

