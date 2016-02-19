/*
 * Copyright (c) 1996-2000 University of Utah and the Flux Group.
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

#include <oskit/x86/pc/com_console.h>
#include <oskit/x86/pc/gdb_console.h>
#include <oskit/x86/debug_reg.h>
#include <oskit/c/stdio.h>
#include <oskit/c/unistd.h>
#include <oskit/c/stdlib.h>
#include <oskit/c/termios.h>
#include <oskit/tty.h>
#include <oskit/gdb.h> 
#include <oskit/gdb_serial.h> /* for gdb_serial_exit */
#include <oskit/base_critical.h>
#include <oskit/config.h>

/* This is set by giving "-d" to the FreeBSD bootblocks */
int enable_gdb = 0;			/* set to 1 for kernel debugging */

extern int    oskit_argc;
extern char **oskit_argv;

#ifdef HAVE_DEBUG_REGS
void gdb_pc_set_bkpt(void *bkpt);
#endif

void
init(void)
{
	int i;
	char *p;
	int rc;

	int gdb_com_port = 1;		/* second serial for gdb */

	/*
	 * XXX: "-f" is a Utah-specific hack to allow FreeBSD bootblocks 
	 * to tell the kernel to run at 115200 instead of the default 9600.
	 * Note: if -f is passed w/o -h, will use the keyboard.
	 * This is done so that "-f" can be hardcoded, and just
	 * change -h to select serial/keyboard.
	 */

	/* Initialize our configuration options from environment variables */
	if ((p = getenv("GDB_COM")) != NULL) {
		gdb_com_port = atoi(p);
		enable_gdb = 1;
	}
	if ((p = getenv("GDB_BAUD")) != NULL
           || (p = getenv("BAUD")) != NULL
           ) {
		base_raw_termios.c_ispeed = atoi(p);
		base_raw_termios.c_ospeed = atoi(p);
	}

	/* Deal with any boot flags that we care about */
	for (i = 0; i < oskit_argc; i++) {
		if (strcmp(oskit_argv[i], "-f") == 0) {
			base_raw_termios.c_ispeed = B115200;
			base_raw_termios.c_ospeed = B115200;
		} else if (strcmp(oskit_argv[i], "-d") == 0) {
			/* enable gdb */
			enable_gdb = 1;
		}
	}

        if (enable_gdb) {
                oskit_stream_t *dummy;
                rc = gdb_console_init(gdb_com_port, &base_raw_termios,
                                      &dummy);

#if 0
                /* Printing error is unlikely to do any good */
                if (rc)
                        panic("error creating gdb_console: %d", rc);
#endif

#ifdef HAVE_DEBUG_REGS
                {
		extern int main(int argc, char **argv);
		gdb_pc_set_bkpt(main);
                }
#else
		/*
		 * Take an immediate breakpoint trap here
		 * so that the debugger can take control from the start.
		 */
		asm("int $3");	/* XXX */
#endif
	}
}

oskit_error_t
fini(void)
{
        int rc; /* ToDo: how can we get hold of the real exit value? */

        /* ToDo: make this part of the gdb finaliser! */
	if (enable_gdb) {
		/* Detach from the remote GDB. */
		gdb_serial_exit(rc);

#ifdef HAVE_DEBUG_REGS
		/* Turn off the debug registers. */
		set_dr7(get_dr7() & ~(DR7_G0 | DR7_G1 | DR7_G2 | DR7_G3));
#endif

	}
        return 0;
}
