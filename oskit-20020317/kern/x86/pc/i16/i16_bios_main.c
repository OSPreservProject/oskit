/*
 * Copyright (c) 1994-1998 University of Utah and the Flux Group.
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
 * Default i16_bios_main() routine for programs starting from the BIOS.
 */

#include <stdlib.h>
#include <oskit/x86/i16.h>
#include <oskit/x86/pc/base_i16.h>
#include <oskit/x86/pc/base_real.h>
#include <oskit/x86/pc/base_console.h>

#ifdef KNIT

extern void start32(void);

#else

CODE32

extern int main(int argc, char *argv[], char *envp[]);
extern char **environ;

static void start32(void)
{
	int argc = 1;
	char *argv[2] = {"kernel", NULL};

	/* Grab all the conventional and extended memory we can find.  */
	base_conv_mem_init();
	base_ext_mem_init();

	/* Get console I/O going */
	base_console_init(argc, argv);

	/* Start the application */
	exit(main(argc, argv, environ));
}

#endif /* !KNIT */

CODE16

void i16_bios_main(void)
{
	/* Initialize the variables describing our virtual memory layout */
	i16_init_vm();

#ifndef KNIT
	/* XXX just to make sure this gets linked in... */
	i16_putchar('\n');
#endif

	i16_raw_start(start32);
}

