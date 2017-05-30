/*
 * Copyright (c) 1997-1998, 2000 University of Utah and the Flux Group.
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
#include <errno.h>
#include <fd.h>

/*
 * fflush
 *
 * Since the oskit stdio doesn't buffer at all, we just return 
 * success.  We do try and check for bogus stream pointers though.
 */
int
fflush(FILE *stream) 
{
	/* NULL isn't really an error, since it means that we're 
	 * supposed to flush all streams.  So just return success.
	 */
	if (stream == NULL)
		return 0;

	if (fd_check_stream(fileno(stream)))
		return EOF;

	return 0;
}
