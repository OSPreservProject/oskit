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
 * Switch to a real tty driver for the console. Query the environment
 * for it, which is somewhat silly since this routine is called directly
 * out of the startup library.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <oskit/io/ttystream.h>
#include <oskit/c/environment.h>
#include "fd.h"

void
oskit_console_reinit(void)
{
	oskit_iunknown_t	*console_stream;
	oskit_error_t		rc;
		
	rc = oskit_libcenv_getconsole(libc_environment,
				      (void *) &console_stream);

	if (rc)
		panic("oskit_console_reinit: No console TTY device");
	
	fd_init();
	fd_free(0);
	fd_free(1);
	fd_free(2);
	fd_alloc((oskit_iunknown_t*)console_stream, 0);
	fd_alloc((oskit_iunknown_t*)console_stream, 1);
	fd_alloc((oskit_iunknown_t*)console_stream, 2);
}
