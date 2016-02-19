/*
 * Copyright (c) 1997-2000 University of Utah and the Flux Group.
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
 * This piece of garbage is here to deal with the fact that the C library
 * might not have a default console set yet. If it does, we need to install
 * that, otherwise need to look it up. In either case, it is necessary for
 * the operations to fds 0,1,2 to proceed normally, without requiring a
 * call to some initializer. All this to ensure that console output works
 * from the getgo in a real oskit kernel.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <oskit/io/asyncio.h>
#include <oskit/io/posixio.h>
#include <oskit/com/stream.h>
#include <oskit/c/environment.h>
#include "fd.h"

static struct oskit_iunknown_ops unknown_ops;
oskit_iunknown_t		 console_stream  = { &unknown_ops };

/*
 * This is a common symbol. If the program is linked with the startup
 * library, its defintion will come from there. Console operations are
 * then chained to that definition. Otherwise, we need to find a console
 * stream to use instead. That will happen when the stream is first
 * queried for its interfaces. By that time, someone had better have
 * installed a console!
 */
struct oskit_stream *default_console_stream;

static OSKIT_COMDECL
query(oskit_iunknown_t *si, const struct oskit_guid *iid, void **out_ihandle)
{
	oskit_iunknown_t *console_stream;
	static int	didit;
	int		i;

	if (didit)
		panic("posix console query reentry!");

	console_stream = (oskit_iunknown_t *) default_console_stream;

#ifndef KNIT
	/*
	 * If there is no console stream compiled in, ask the Client OS.
	 */
	if (!console_stream) {
		oskit_error_t	rc;
		
		rc = oskit_libcenv_getconsole(libc_environment,
					      (void *) &console_stream);

		if (rc)
			exit(rc);
	}
#endif

	/*
	 * Make sure we are (at least) answering the query
	 * that was asked of us.  This is gross, but not
	 * nearly as gross as not doing the assert...
	 */
	 
	assert((iid == &oskit_iunknown_iid)
	       || (iid == &oskit_stream_iid)
	       || (iid == &oskit_posixio_iid)
	       || (iid == &oskit_asyncio_iid)
	       || (iid == &oskit_ttystream_iid));

	assert((out_ihandle == (void**)(&fd_array[0].obj))
	       || (out_ihandle == (void**)(&fd_array[0].stream))
	       || (out_ihandle == (void**)(&fd_array[0].posixio))
	       || (out_ihandle == (void**)(&fd_array[0].asyncio))
	       || (out_ihandle == (void**)(&fd_array[0].ttystream))
	       || (out_ihandle == (void**)(&fd_array[1].obj))
	       || (out_ihandle == (void**)(&fd_array[1].stream))
	       || (out_ihandle == (void**)(&fd_array[1].posixio))
	       || (out_ihandle == (void**)(&fd_array[1].asyncio))
	       || (out_ihandle == (void**)(&fd_array[1].ttystream))
	       || (out_ihandle == (void**)(&fd_array[2].obj))
	       || (out_ihandle == (void**)(&fd_array[2].stream))
	       || (out_ihandle == (void**)(&fd_array[2].posixio))
	       || (out_ihandle == (void**)(&fd_array[2].asyncio))
	       || (out_ihandle == (void**)(&fd_array[2].ttystream)));

	/*
	 * Just pre-cache all of the possible iid queries that could
	 * be made (instead of just doing the one that was asked for).
	 */
	for (i = 0; i < 3; i++) {
		oskit_iunknown_query(console_stream,
				     &oskit_iunknown_iid,
				     (void **) &fd_array[i].obj);

		oskit_iunknown_query(console_stream,
				     &oskit_stream_iid,
				     (void **) &fd_array[i].stream);

		oskit_iunknown_query(console_stream,
				     &oskit_posixio_iid,
				     (void **) &fd_array[i].posixio);

		oskit_iunknown_query(console_stream,
				     &oskit_ttystream_iid,
				     (void **) &fd_array[i].ttystream);

		oskit_iunknown_query(console_stream,
				     &oskit_asyncio_iid,
				     (void **) &fd_array[i].asyncio);

		oskit_iunknown_query(console_stream,
				     &oskit_ttystream_iid,
				     (void **) &fd_array[i].ttystream);
	}

	/*
	 * Will never return to this routine again ...
	 */
	didit = 1;

	return 0;
}

static OSKIT_COMDECL_U
addref(oskit_iunknown_t *si)
{
	panic("console ops addref");
	return 1;
}

static OSKIT_COMDECL_U
release(oskit_iunknown_t *si)
{
	panic("console ops release");
	return 1;
}

static struct oskit_iunknown_ops unknown_ops = {
	query,
	addref,
	release
};
