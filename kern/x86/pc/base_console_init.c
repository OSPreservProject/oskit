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

#include <oskit/x86/pc/base_console.h>
#include <oskit/x86/pc/direct_console.h>
#include <oskit/x86/pc/com_console.h>
#include <oskit/x86/pc/gdb_console.h>
#include <oskit/x86/pc/reset.h>
#include <oskit/x86/base_gdt.h>
#include <oskit/x86/debug_reg.h>
#include <oskit/x86/base_paging.h>
#include <oskit/x86/base_cpu.h>
#include <oskit/c/stdio.h>
#include <oskit/c/unistd.h>
#include <oskit/c/stdlib.h>
#include <oskit/c/string.h>
#include <oskit/c/termios.h>
#include <oskit/tty.h>
#include <oskit/gdb.h>
#include <oskit/gdb_serial.h> /* for gdb_serial_exit */
#include <oskit/base_critical.h>
#include <oskit/config.h>

#ifdef HAVE_DEBUG_REGS
void gdb_pc_set_bkpt(void *bkpt);
#endif

extern oskit_stream_t boot_direct_console;
oskit_stream_t *console = &boot_direct_console;
                                        /* Currently selected console      */

/* This is set if RB_SERIAL is passed by the FreeBSD bootblocks */
int serial_console = 0;			/* set to 1 to use serial comsole, */
					/*   0 to use keyboard */
/* This is set by giving "-d" to the FreeBSD bootblocks */
int enable_gdb = 0;			/* set to 1 for kernel debugging */

/* This is set by giving "-p" to the FreeBSD bootblocks */
/* XXX it's hardwired for now (!).  The bootmanager won't
 * pass in flags it's not aware of, like -p */
int enable_gprof = 1;                   /* set to 1 for kernel profiling */

#ifndef KNIT

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
	extern oskit_addr_t return_address;

	printf("_exit(%d) called; %s...\r\n",
	       rc, return_address ? "returning to netboot" : "rebooting");

	if (enable_gdb) {
		/* Detach from the remote GDB. */
		gdb_serial_exit(rc);

#ifdef HAVE_DEBUG_REGS
		/* Turn off the debug registers. */
		set_dr7(get_dr7() & ~(DR7_G0 | DR7_G1 | DR7_G2 | DR7_G3));
#endif

	}

	/* flush and wait for `_exit called` message */
	oskit_stream_release(console);
	if (!serial_console) {
		/* This is so that the user has a chance to SEE the output */
		printf("Press a key to reboot");
		getchar();
	}

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
	else
		pc_reset();
}
#endif /* ! KNIT */

/*
 * This function parses the multiboot command line and
 * initializes the serial lines.
 */
void
base_console_init(int argc, char **argv)
{
	int i;
	char *p;
	int rc;

 	int cons_com_port = 1;	        /* first serial (or screen) console */
	int gdb_com_port = 1;		/* second serial for gdb */
	int enable_killswitch = 0;      /* Kill switch using second tipline. */

	/*
	 * XXX: "-f" is a Utah-specific hack to allow FreeBSD bootblocks
	 * to tell the kernel to run at 115200 instead of the default 9600.
	 * Note: if -f is passed w/o -h, will use the keyboard.
	 * This is done so that "-f" can be hardcoded, and just
	 * change -h to select serial/keyboard.
	 */

	/* Initialize our configuration options from environment variables */
	if ((p = getenv("CONS_COM")) != NULL) {
		cons_com_port = atoi(p);
		serial_console = 1;
	}
	if ((p = getenv("GDB_COM")) != NULL) {
		gdb_com_port = atoi(p);
		enable_gdb = 1;
	}
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
			/* RB_SERIAL console flag from the BSD boot blocks */
			serial_console = 1;
		} else if (strcmp(argv[i], "-d") == 0) {
			/* enable gdb/gdb */
			enable_gdb = 1;
		} else if (strcmp(argv[i], "-p") == 0) {
			/* enable gprof */
			enable_gprof = 1;
		} else if (strcmp(argv[i], "-k") == 0) {
			/* enable killswitch */
			enable_killswitch = 1;
		}
	}

	if (getenv("DIRECT_CONS"))
		serial_console = 0;

	/* Initialize the serial console, if we are using it */
	if (serial_console) {
		rc = com_console_init(cons_com_port, &base_cooked_termios,
				      &console);
	} else {
		if (enable_gdb) {
			oskit_stream_t *dummy;
			rc = gdb_console_init(gdb_com_port, &base_raw_termios,
					      &dummy);
		}
		rc = direct_console_init(&console);
	}
	/*
	 * Initialise gdb - console will get overwritten if
	 * gdb_com_port == cons_com_port
	 */
	if (enable_gdb) {
		oskit_stream_t *dummy;
		rc = gdb_console_init(gdb_com_port, &base_raw_termios,
				      &dummy);
		if (gdb_com_port == cons_com_port) {
			console = dummy;
		}
	}

#if 0
	/* Printing error is unlikely to do any good */
	if (rc)
		panic("error creating console: %d", rc);
#endif

	/* Initialize GDB if it is enabled */
	if (enable_gdb) {
#ifdef HAVE_DEBUG_REGS
		extern int main(int argc, char **argv);
		gdb_pc_set_bkpt(main);
#else
		/*
		 * Take an immediate breakpoint trap here
		 * so that the debugger can take control from the start.
		 */
		asm("int $3");	/* XXX */
#endif
	}

	/*
	 * This gross hack, allows the second tipline (gdb line) to be
	 * used as a kill switch to force the kernel into a panic. Very
	 * useful when the kernel is looping someplace and interrupts are
	 * still enabled. Simply cat some characters into the special
	 * device file (/dev/tip/foo-gdb).
	 */
	if (enable_killswitch && !enable_gdb &&
	    gdb_com_port != cons_com_port) {
		extern void gdb_pc_set_killswitch(int port);
		gdb_pc_set_killswitch(gdb_com_port);
	}
}
