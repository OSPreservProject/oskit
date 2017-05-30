/*
 * Copyright (c) 1997-1998, 2000 University of Utah and the Flux Group.
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
#include <oskit/console.h>

#include "native.h"
#include <unistd.h>

#undef	EOF
#define EOF (-1)

/* Stdio init/ext does nothing in single threaded mode */
void oskitunix_stdio_init() {}
void oskitunix_stdio_exit() {}

/* Supply console routines normally found in libkern. */

int
console_puts(const char *str)
{
	int rc;

	rc = NATIVEOS(write)(1, str, strlen(str));
	if (rc <= 0)
		return EOF;
	else
		return console_putchar('\n');
}

int
console_putchar(int c)
{
	unsigned char foo = (unsigned char) c;

	if (NATIVEOS(write)(1, &foo, 1) <= 0)
		return EOF;
	else
		return c;
}

int
console_getchar()
{
	unsigned char foo;

	if (NATIVEOS(read)(0, &foo, 1) <= 0)
		return EOF;
	else
		return (int) foo;
}

/*
 * Not overridden by native libc. Added to improve console output performance.
 */
int
console_putbytes(const char *str, int len)
{
	int rc;

	rc = NATIVEOS(write)(1, str, len);
	if (rc <= 0)
		return EOF;
	else
		return 0;
}


void
base_console_init(int argc, char **argv)
{
    console_putbytes("base_console_init called!", strlen("base_console_init called!"));
}
