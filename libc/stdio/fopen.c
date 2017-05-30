/*
 * Copyright (c) 1994-1995, 1998 University of Utah and the Flux Group.
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
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

FILE *
fopen(const char *name, const char *mode)
{
	int base_mode = O_RDONLY, flags = 0;
	FILE *stream;

	/* Find the appropriate Unix mode flags.  */
	switch (*mode++)
	{
		case 'w':
			base_mode = O_WRONLY;
			flags |= O_CREAT | O_TRUNC;
			break;
		case 'a':
			base_mode = O_WRONLY;
			flags |= O_CREAT | O_APPEND;
			break;
	}
	while (*mode)
	{
		if (*mode == '+')
			base_mode = O_RDWR;
		mode++;
	}

	if (!(stream = malloc(sizeof(*stream))))
	{
		errno = ENOMEM;
		return 0;
	}

	if ((stream->_file = open(name, base_mode | flags)) < 0)
	{
		free(stream);
		return 0;
	}

	stream->_flags &= ~(__SEOF|__SERR);
	return stream;
}

