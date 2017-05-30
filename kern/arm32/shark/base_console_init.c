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

#include <oskit/arm32/ofw/ofw.h>
#include <oskit/arm32/ofw/ofw_console.h>
#include <oskit/arm32/shark/base_console.h>
#include <oskit/arm32/shark/com_console.h>
#include <oskit/arm32/shark/direct_console.h>
#include <oskit/arm32/shark/gdb_console.h>
#include <oskit/arm32/proc_reg.h>
#include <oskit/arm32/reset.h>
#include <oskit/arm32/gdb.h>
#include <oskit/gdb_serial.h> /* for gdb_serial_exit */
#include <oskit/c/stdio.h>
#include <oskit/c/unistd.h>
#include <oskit/c/stdlib.h>
#include <oskit/c/termios.h>
#include <oskit/tty.h>
#include <oskit/base_critical.h>
#include <oskit/config.h>

oskit_stream_t *console;	/* Currently selected console      */

/* This is set if RB_SERIAL is passed by the FreeBSD bootblocks */
int serial_console = 1;			/* set to 1 to use serial comsole, */
					/*   0 to use keyboard */

/* This is set by giving "-d" to the FreeBSD bootblocks */
int enable_gdb = 0;			/* set to 1 for kernel debugging */

static void our_exit(int rc);
/* A common symbol that is overridden */
void (*oskit_libc_exit)(int) = our_exit;

/*
 * This overrides the _exit() function in libc.
 * If the serial console (or remote GDB) is being used, it waits
 * until all the data has cleared out of the FIFOs; if the VGA 
 * display is being used (normal console), then it waits for a keypress.
 * When it is done, it calls pc_reset() to reboot the computer.
 */
static void
our_exit(int rc)
{
	if (enable_gdb) {
		/* Detach from the remote GDB. */
		gdb_serial_exit(rc);
	}

	printf("_exit(%d) called; rebooting...\n", rc);
	disable_interrupts();
	arm32_reset();
	console_getchar();
}

/*
 * This function parses the multiboot command line and
 * initializes the serial lines.
 */
void
base_console_init(int argc, char **argv)
{
	int i, rc;
	char *p;

 	int cons_com_port = 1;	        /* first serial (or screen) console */
	int gdb_com_port = 1;		/* only one serial line */

	/*
	 * XXX: "-f" is a Utah-specific hack to allow FreeBSD bootblocks 
	 * to tell the kernel to run at 115200 instead of the default 9600.
	 * Note: if -f is passed w/o -h, will use the keyboard.
	 * This is done so that "-f" can be hardcoded, and just
	 * change -h to select serial/keyboard.
	 */

	/* Initialize our configuration options from environment variables */
	if ((p = getenv("BAUD")) != NULL) {
		base_cooked_termios.c_ispeed = atoi(p);
		base_cooked_termios.c_ospeed = atoi(p);
		base_raw_termios.c_ispeed = atoi(p);
		base_raw_termios.c_ospeed = atoi(p);
	}

	/* Deal with any boot flags that we care about */
	for (i = 0; i < argc; i++) {
		if (strcmp(argv[i], "-f") == 0) {
			base_cooked_termios.c_ispeed = B115200;
			base_cooked_termios.c_ospeed = B115200;
			base_raw_termios.c_ispeed = B115200;	/* gdb */
			base_raw_termios.c_ospeed = B115200;
		} else if (strcmp(argv[i], "-h") == 0) {
			serial_console = 1;
		} else if (strcmp(argv[i], "-d") == 0) {
			/* enable gdb/gdb */
			enable_gdb = 1;
			base_raw_termios.c_ispeed = B115200;	/* gdb */
			base_raw_termios.c_ospeed = B115200;
		}
	}

	if (serial_console) {
		rc = com_console_init(cons_com_port, &base_cooked_termios,
				      &console);
		if (enable_gdb) {
			oskit_stream_t *dummy;
			rc = gdb_console_init(gdb_com_port, &base_raw_termios,
					      &dummy);
			if (gdb_com_port == cons_com_port) {
				console = dummy;
			}
		}
	}
	else {
		if (enable_gdb) {
			oskit_stream_t *dummy;
			rc = gdb_console_init(gdb_com_port, &base_raw_termios,
					      &dummy);
		}
		rc = direct_console_init(&console);
	}

	/* Drop into GDB if enabled */
	if (enable_gdb) {
		extern int main(int argc, char **argv);
		gdb_set_bkpt(main);
	}
}
