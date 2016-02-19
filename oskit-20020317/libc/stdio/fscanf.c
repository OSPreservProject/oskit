/*
 * Copyright (c) 1994-1998 The University of Utah and the Flux Group.
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
 * Mach Operating System
 * Copyright (c) 1993-1989 Carnegie Mellon University.
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */

#include <stdio.h>
#include <stdarg.h>
#include "doscan.h"

/* XXX kludge to work around absence of ungetc() */
typedef struct state
{
	FILE *fh;
	int ch;
} state;

static int
read_char(void *arg)
{
	state *st = (state*)arg;
	if (st->ch >= 0)
	{
		int c = st->ch;
		st->ch = -1;
		return c;
	}
	else
		return fgetc(st->fh);
}

static void
unread(int c, void *arg)
{
	state *st = (state*)arg;
	st->ch = c;
}

#if 0
static int
read_char(void *arg)
{
	return fgetc((FILE*)arg);
}

static void
unread(int c, void *arg)
{
	ungetc(c, (FILE*)arg);
}
#endif

int vfscanf(FILE *fh, const char *fmt, va_list args)
{
	return _doscan(fmt, args, read_char, unread, fh);
}

int
fscanf(FILE *fh, const char *fmt, ...)
{
	int nmatch = 0;
	va_list	args;
	state st;

	st.fh = fh;
	st.ch = -1;

	va_start(args, fmt);
	nmatch = vfscanf((FILE *)&st, fmt, args);	
	va_end(args);

	return nmatch;
}

