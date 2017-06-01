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

#include <stdlib.h>
#include <oskit/x86/i16.h>
#include <oskit/x86/pc/i16_dos.h>
#include <oskit/x86/pc/base_i16.h>
#include <oskit/x86/pc/base_real.h>
#include <oskit/x86/pc/base_console.h>

extern int main(int argc, char *argv[], char *envp[]);
extern char **environ;

static int argc;
static char **argv;

CODE32

static void start32(void)
{
	/* Grab all the conventional and extended memory we can find.  */
	/*base_dos_mem_init(); XXX*/
	base_ext_mem_init();

	/* Get console I/O going */
	base_console_init(argc, argv);

	/* Start the application */
	exit(main(argc, argv, environ));
}

CODE16

void i16_dos_main(int _argc, char **_argv)
{
	argc = _argc;
	argv = _argv;

	i16_init_vm();

	/* XXX just to make sure this gets linked in... */
	i16_putchar('\n');

	/* Make sure we're running on a good enough DOS version.  */
	if (i16_dos_version() < 0x300)
		i16_panic("DOS 3.00 or higher required.");

#if 0
	/* See if we're running in a DPMI or VCPI environment.
	   If either of these are successful, they don't return.  */
	i16_dos_mem_check();
#ifdef ENABLE_DPMI
	i16_dpmi_check();
#endif
	i16_xms_check();
	i16_ext_mem_check();
#ifdef ENABLE_VCPI
	i16_vcpi_check();
#endif
#endif

	i16_raw_start(start32);
}

