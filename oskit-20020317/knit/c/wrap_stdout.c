/*
 * Copyright (c) 2000 University of Utah and the Flux Group.
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

#include <stdio.h>
#include <stdarg.h>

extern void pre(void);
extern void post(void);

// extern int in_vprintf(const char *fmt, va_list args);

extern typeof(vprintf) in_vprintf;
extern typeof(printf)  in_printf;
extern typeof(puts)    in_puts;
extern typeof(putchar) in_putchar;

extern typeof(vprintf) out_vprintf;
extern typeof(printf)  out_printf;
extern typeof(puts)    out_puts;
extern typeof(putchar) out_putchar;

int out_vprintf(const char *fmt, va_list args)
{
	int rc;
        pre();
	rc = in_vprintf(fmt, args);
        post();
	return rc;
}

int out_printf(const char *fmt, ...)
{
	va_list	args;
	int rc;

	va_start(args, fmt);
	rc = out_vprintf(fmt, args);
	va_end(args);

	return rc;
}

int out_puts(const char *s)
{
	int rc;
        pre();
	rc = in_puts(s);
        post();
	return rc;
}

int out_putchar(int c)
{
        int rc;
        pre();
	rc = in_putchar(c);
        post();
        return rc;
}
