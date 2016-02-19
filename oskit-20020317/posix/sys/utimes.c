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

#include <unistd.h>
#include <sys/time.h>
#include "fs.h"

int
utimes(const char *name, const struct timeval tvp[2])
{
	oskit_file_t *f;
	oskit_error_t rc = fs_lookup(name, FSLOOKUP_FOLLOW, &f);

	if (!rc) {
		struct oskit_stat sb;
		TIMEVAL_TO_TIMESPEC(&tvp[0], &sb.atime);
		TIMEVAL_TO_TIMESPEC(&tvp[1], &sb.mtime);
		rc = oskit_file_setstat(f,
					OSKIT_STAT_ATIME|OSKIT_STAT_MTIME,
					&sb);
		oskit_file_release(f);
	}
	return rc ? (errno = rc), -1 : 0;
}
