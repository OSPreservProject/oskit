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

#include <oskit/error.h>
#include <oskit/x86/eflags.h>

#include "dos_io.h"

int dos_check_err(struct trap_state *ts)
{
	if (ts->eflags & EFL_CF) {
		switch (ts->eax & 0xffff) {
		case 0x01:	return OSKIT_EINVAL;
		case 0x02:	return OSKIT_ENOENT;
		case 0x03:	return OSKIT_ENOENT;
		case 0x04:	return OSKIT_EMFILE;
		case 0x05:	return OSKIT_EACCES;
		case 0x06:	return OSKIT_EINVAL;
		case 0x08:	return OSKIT_ENOMEM;
		case 0x09:	return OSKIT_EINVAL;
		case 0x0a:	return OSKIT_EINVAL;
		case 0x0b:	return OSKIT_EINVAL;
		case 0x0c:	return OSKIT_EINVAL;
		case 0x0d:	return OSKIT_EINVAL;
		case 0x0f:	return OSKIT_EINVAL;
		case 0x11:	return OSKIT_EXDEV;
		case 0x12:	return OSKIT_ENOENT;
		case 0x13:	return OSKIT_EROFS;
		case 0x15:	return OSKIT_EIO;
		case 0x16:	return OSKIT_EINVAL;
		case 0x17:	return OSKIT_EIO;
		case 0x18:	return OSKIT_EINVAL;
		case 0x19:	return OSKIT_EIO;
		case 0x1a:	return OSKIT_EIO;
		case 0x1b:	return OSKIT_EIO;
		case 0x1c:	return OSKIT_EIO;
		case 0x1d:	return OSKIT_EIO;
		case 0x1e:	return OSKIT_EIO;
		case 0x27:	return OSKIT_ENOSPC;
		case 0x34:	return OSKIT_EEXIST;
		case 0x35:	return OSKIT_EINVAL;
		case 0x50:	return OSKIT_EEXIST;
		default:	return OSKIT_E_UNEXPECTED;
		}
	}
	else
		return 0;
}

