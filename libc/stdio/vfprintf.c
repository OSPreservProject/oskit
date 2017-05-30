/*
 * Copyright (c) 1994-1996, 1998 University of Utah and the Flux Group.
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
#include <unistd.h>
#include <stdarg.h>
#include "doprnt.h"

#define	PRINTF_BUFMAX	128

struct printf_state {
	FILE *stream;
	char buf[PRINTF_BUFMAX];
	unsigned int index;
};


static void
flush(struct printf_state *state)
{
	if (state->index > 0)
	{
		write(fileno(state->stream), state->buf, state->index);
		state->index = 0;
	}
}

static void
dochar(void *arg, int c)
{
	struct printf_state *state = (struct printf_state *) arg;

	if (state->index >= PRINTF_BUFMAX)
		flush(state);

	state->buf[state->index] = c;
	state->index++;
}

/*
 * Printing (to console)
 */
int vfprintf(FILE *stream, const char *fmt, va_list args)
{
	struct printf_state state;

	state.stream = stream;
	state.index = 0;
	_doprnt(fmt, args, 0, (void (*)())dochar, (char *) &state);

	flush(&state);

	return 0;
}

