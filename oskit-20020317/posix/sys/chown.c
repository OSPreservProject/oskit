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
#include "fs.h"

int
chown(const char *name, oskit_uid_t uid, oskit_gid_t gid)
{
	oskit_file_t *f;
	oskit_error_t rc = fs_lookup(name, FSLOOKUP_FOLLOW, &f);

	if (!rc) {
		struct oskit_stat sb;
		oskit_u32_t mask = 0;
		sb.uid = uid;
		sb.gid = gid;
		if (uid != -1)
			mask |= OSKIT_STAT_UID;
		if (gid != -1)
			mask |= OSKIT_STAT_GID;

		rc = oskit_file_setstat(f, mask, &sb);
		oskit_file_release(f);
	}
	return rc ? (errno = rc), -1 : 0;
}

