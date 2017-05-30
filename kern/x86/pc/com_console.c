/*
 * Copyright (c) 1997-1999 University of Utah and the Flux Group.
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
 * IStream wrapper for com console
 */

#include <oskit/com/stream.h>
#include <oskit/com/trivial_stream.h>

#include <oskit/x86/pc/com_cons.h>

static int com_cons_port;
static int cons_getchar(void) { return com_cons_getchar(com_cons_port); }
static int cons_putchar(int c)
{ com_cons_putchar(com_cons_port, c); return (unsigned char) c; }

static struct oskit_trivial_stream com_console_impl = {
	streami: { &oskit_trivial_stream_ops },
	getchar: cons_getchar,
	putchar: cons_putchar
};

oskit_error_t
com_console_init(int com_port, struct termios *com_params,
		 oskit_stream_t **out_stream)
{
	com_cons_init(com_port, com_params);
	com_cons_port = com_port;
	*out_stream = &com_console_impl.streami;
	return 0;
}
