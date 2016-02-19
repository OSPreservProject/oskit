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
 * Define console routines so we can use printf.
 *
 * We could probably get away with using the console code from libkern's
 * base_console.c if we consed up an argv/argc from our boothowto and
 * called base_console_init with them.
 */

#include <oskit/x86/pc/com_cons.h>
#include <oskit/x86/pc/direct_cons.h>
#include <oskit/x86/pc/base_console.h>
#include <oskit/x86/pc/reset.h>

#include <oskit/tty.h>
#include <oskit/console.h>
#include <oskit/c/termios.h>
#include <oskit/c/stdlib.h>


/*
 * Define this if you want the boot adaptor to always use the serial
 * line for the console,
 * regardless of what the RB_SERIAL flag (if any) is set to.
 */
#undef BA_FORCE_SERIAL

static int serial = 0;			/* Use serial console or not. */
static int cons_com_port = 1;

static void our_exit(int rc);
/* A common symbol that is overridden */
extern void (*oskit_libc_exit)(int) = our_exit;

/*
 * This overrides the _exit() function in libc.
 * When it is done, it calls pc_reset() to reboot the computer.
 */
static void
our_exit(int rc)
{
	if (serial)
		com_cons_flush(cons_com_port);

	pc_reset();
}

static void
putchar_init()
{
	/*
	 * XXX Since this is linux, we'll need to check the kernel command
	 * line.
	 */
#ifdef BA_FORCE_SERIAL
	serial = 1;
#else
	serial = 0;
#endif

	if (serial) {
		if (0) {
			base_cooked_termios.c_ispeed = B115200;
			base_cooked_termios.c_ospeed = B115200;
		}
		com_cons_init(cons_com_port, &base_cooked_termios);
	}
}

int
console_putchar(int c)
{
	static int inited = 0;

	if (! inited) {
		putchar_init();
		inited = 1;
	}

	serial ? com_cons_putchar(cons_com_port, c) : direct_cons_putchar(c);

	return c;
}

/*
 * This is more efficient for console output, and allows similar treatment
 * in usermode where character based output is really bad.
 */
int
console_putbytes(const char *s, int len)
{
	while (len) {
		console_putchar(*s++);
		len--;
	}
	return 0;
}

/*
 * Provided for completeness, and to avoid linker problems.
 */
int
console_getchar(void)
{
	panic("console_getchar: Linux boot adaptor stub version");
}


/* Nuke the direct console pc speaker bell. */
void
direct_cons_bell(void)
{
}

/* Also do nothing for base_console_init */
void
base_console_init(int argc, char **argv)
{
}
