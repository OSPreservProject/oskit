/*
 * Copyright (c) 1994-1999 The University of Utah and the Flux Group.
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
 * Copyright (c) 1993,1991,1990,1989 Carnegie Mellon University
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
#include <oskit/com/stream.h>
#include "doprnt.h"

/*
 * This version of printf prints to a COM stream object.
 */

#define	PRINTF_BUFMAX	128

struct printf_state {
	oskit_stream_t *stream;
	char buf[PRINTF_BUFMAX];
	unsigned int index;
};

static void
flush(struct printf_state *state)
{
	oskit_size_t n;
	oskit_stream_write(state->stream, state->buf, state->index, &n);
	state->index = 0;
}

static void
printf_char(arg, c)
	char *arg;
	int c;
{
	struct printf_state *state = (struct printf_state *) arg;

	state->buf[state->index++] = (unsigned char) c;
	if (c == 0 || c == '\n' || state->index == PRINTF_BUFMAX)
		flush(state);
}

int com_vprintf(oskit_stream_t *stream, const char *fmt, va_list args)
{
	struct printf_state state;

	state.stream = stream;
	state.index = 0;
	_doprnt(fmt, args, 0, (void (*)())printf_char, (char *) &state);

	if (state.index != 0)
	    flush(&state);

	/* _doprnt currently doesn't pass back error codes,
	   so just assume nothing bad happened.  */
	return 0;
}

int
com_printf(oskit_stream_t *stream, const char *fmt, ...)
{
	va_list	args;
	int err;

	va_start(args, fmt);
	err = com_vprintf(stream, fmt, args);
	va_end(args);

	return err;
}
