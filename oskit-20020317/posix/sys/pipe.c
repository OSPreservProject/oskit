/*
 * Copyright (c) 1997-1998 University of Utah and the Flux Group.
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

#include <oskit/com/pipe.h>
#include <unistd.h>
#include "fd.h"

int
pipe(int *fds)
{
	int		rc;
	int		fd0, fd1;
	oskit_pipe_t	*pipe0, *pipe1;

	rc = oskit_create_pipe(&pipe0, &pipe1);

	if (rc) {
		errno = rc;
		return -1;
	}
	
	/* assign file descriptors to the new pipe objects */
        fd0 = fd_alloc((oskit_iunknown_t*)pipe0, 0);
	if (fd0 < 0) {
		oskit_pipe_release(pipe0);
		oskit_pipe_release(pipe1);
		return -1;
	}
	
        fd1 = fd_alloc((oskit_iunknown_t*)pipe1, 0);
	if (fd1 < 0) {
		fd_free(fd0);
		oskit_pipe_release(pipe1);
		return -1;
	}
	oskit_pipe_release(pipe0);
	oskit_pipe_release(pipe1);
	
	fds[0] = fd0;
	fds[1] = fd1;

	return 0;
}

