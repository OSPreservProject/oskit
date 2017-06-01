/*
 * Copyright (c) 1996-1998, 2000 University of Utah and the Flux Group.
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

char *
fgets(char *str, int size, FILE *stream)
{
	int i;
	int c;

	if (size <= 0)
		return NULL;

	/*
	 * Special case for stdin to call read for the whole size,
	 * since console read will stop at the newline and do line
	 * discipline stuff for us.
	 */
	if (stream == stdin) {
		int rc = read(fileno(stream), str, size);
		if (rc > 0)
			return str;
		if (rc < 0)
			stream->_flags |= __SERR;
		else
			stream->_flags |= __SEOF;
		return NULL;
	}

	i = 0;
	while (i < size) {
		c = fgetc(stream);
		if (c == EOF) {
			if (i == 0)
				return NULL;
			break;
		}
		str[i++] = c;
		if (c == '\n')
			break;
	}
	if (i < size)
		str[i] = '\0';
	return str;
}
