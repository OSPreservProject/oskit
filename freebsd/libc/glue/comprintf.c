/*
 * Copyright (c) 1999 The University of Utah and the Flux Group.
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
 * This version of printf prints to a COM stream object.
 */

#include <stdio.h>
#include <stdarg.h>
#include <errno.h>

#include <oskit/error.h>
#include <oskit/com/stream.h>


/*
 * Helper function for stdio "cookie" i/o -> COM stream i/o.
 */
static int
writefn(void *cookie, const char *buf, int count)
{
	oskit_stream_t *stream = cookie;
	oskit_u32_t wrote;
	oskit_error_t rc = oskit_stream_write(stream, buf, count, &wrote);
	if (OSKIT_FAILED(rc)) {
		errno = rc;
		return -1;
	}
	return wrote;
}


int
com_vprintf(oskit_stream_t *stream, const char *fmt, va_list args)
{
	FILE f;

	f._p = NULL;		/* no current pointer */
	f._w = 0;		/* nothing to read or write */
	f._r = 0;
	f._lbfsize = 0;	/* not line buffered */
	f._file = -1;		/* no file */
	f._ub._base = NULL;	/* no ungetc buffer */
	f._ub._size = 0;
	f._lb._base = NULL;	/* no line buffer */
	f._lb._size = 0;

	f._cookie = stream;
	f._write = writefn;

	/*
	 * Make the stream unbuffered.
	 * vfprintf will do its own buffering.
	 */
	f._flags = __SWR | __SNBF;
	f._bf._base = f._p = f._nbuf;
	f._bf._size = 1;

	return vfprintf(&f, fmt, args);
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
