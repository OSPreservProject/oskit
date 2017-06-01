/*
 * Copyright (c) 1996-1998 University of Utah and the Flux Group.
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
 * Overrides things from base_console.c with ones that don't have
 * serial console or GDB support.
 * Saves 10k bytes off of the default version.
 */

#include <oskit/x86/pc/direct_cons.h>
#include <oskit/x86/pc/reset.h>
#include <oskit/c/stdio.h>
#include <oskit/c/unistd.h>
#include <oskit/c/stdlib.h>

static void our_exit(int rc);
/* A common symbol that is overridden */
void (*oskit_libc_exit)(int) = our_exit;

/*
 * This function defines our kernel "console device";
 * calls to printf() will eventually call putchar().
 * This implementation simply writes characters to the local PC display.
 */
int
console_putchar(int c)
{
	direct_cons_putchar(c);
	return (c);
}

/*
 * Here we provide the means to read a character.
 */
int
console_getchar(void)
{
	return direct_cons_getchar();
}

int
console_putbytes(const char *s, int len)
{
	while (len) {
		direct_cons_putchar(*s++);
		len--;
	}
	return 0;
}

int
trygetchar(void)
{
	return direct_cons_trygetchar();
}

/*
 * This overrides the _exit() function in libc.
 * Waits for a keypress.
 * When it is done, it calls pc_reset() to reboot the computer.
 */
static void our_exit(int rc)
{
	printf(__FUNCTION__"(%d) called; rebooting...\n", rc);

	/* This is so that the user has a chance to SEE the output */
	printf("Press a key to reboot");
	getchar();

	pc_reset();
}

void
base_console_init(int argc, char **argv)
{
}
