/*
 * Copyright (c) 1996, 1998 University of Utah and the Flux Group.
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
 * Routine for making a libexec error code human-readable.
 */

#include <oskit/c/stdio.h>
#include <oskit/exec/exec.h>

/*
 * Note that this may return a pointer into static storage.
 * This is ok since this is usually used for printf'ing
 */
char *
exec_strerror(int err)
{
	static char buf[128];

	switch (err) {
	case EX_NOT_EXECUTABLE:	return "Invalid format";
	case EX_WRONG_ARCH:	return "Wrong architecture";
	case EX_CORRUPT:	return "Corrupted file";
	case EX_BAD_LAYOUT:	return "Bad layout";
	default:
		sprintf(buf, "Unknown exec error: %d", err);
		return buf;
	}
}
