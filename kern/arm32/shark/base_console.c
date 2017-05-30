/*
 * Copyright (c) 1996-1999 University of Utah and the Flux Group.
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
 * Default console implementation code for kernels on the x86.
 */

#include <oskit/arm32/shark/base_console.h>
#include <oskit/base_critical.h> /* for EOF */
#include <oskit/c/stdio.h>
#include <oskit/config.h>

/*
 * This function defines our kernel "console device";
 * calls to printf() will eventually call putchar().
 * Our implementation simply writes characters to the local PC display,
 * or the serial line, depending on the info from the bootblocks.
 */
int
console_putchar(int c)
{
	int rc;
	oskit_u32_t actual;
	rc = oskit_stream_write(console, &c, 1, &actual);
#if 0 /* Yoiks, no way to print error - ignore rc and actual */
	if (rc || actual != 1)
		panic("error writing to console: %d", rc);
#endif
	return (c);
}

/*
 * Here we provide the means to read a character.
 */
int
console_getchar(void)
{
	char c;
	int  rc;
	oskit_u32_t actual;
	
	rc = oskit_stream_read(console, &c, 1, &actual);
	if (rc == 0 && actual == 1) {
		return c;
	}
	/* Yoiks, no way to print error */
	return EOF;
}

/*
 * While it is not necessary to override puts, this allows us to handle
 * gdb printfs much more efficiently by packaging them together.
 * If we haven't enabled the debugger, we just spit them all out.
 */
int
console_puts(const char *s)
{
	int rc;
	oskit_u32_t actual;

	base_critical_enter();
	rc = oskit_stream_write(console, s, strlen(s), &actual);
	base_critical_leave();

#if 0 /* Yoiks, no way to print error - ignore rc and actual */
	if (rc || actual != len)
		panic("error writing to console: %d", rc);
#endif
	return 0;
}

/*
 * This is more efficient for console output, and allows similar treatment
 * in usermode where character based output is really bad.
 */
int
console_putbytes(const char *s, int len)
{
	int rc;
	oskit_u32_t actual;

	base_critical_enter();
	rc = oskit_stream_write(console, s, len, &actual);
	base_critical_leave();

#if 0 /* Yoiks, no way to print error - ignore rc and actual */
	if (rc || actual != len)
		panic("error writing to console: %d", rc);
#endif
	return 0;
}

