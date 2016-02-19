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
#ifndef	_OSKIT_C_SYS_TYPES_H_
#define	_OSKIT_C_SYS_TYPES_H_

#include <oskit/types.h>
#include <oskit/time.h>

#ifndef _SIZE_T
#define _SIZE_T
typedef oskit_size_t size_t;
#endif

#ifndef _SSIZE_T
#define _SSIZE_T
typedef oskit_ssize_t ssize_t;
#endif

typedef	oskit_dev_t	dev_t;		/* device id */
typedef	oskit_gid_t	gid_t;		/* group id */
typedef	oskit_ino_t	ino_t;		/* inode number */
typedef	oskit_mode_t	mode_t;		/* permissions */
typedef	oskit_nlink_t	nlink_t;	/* link count */
typedef	oskit_off_t	off_t;		/* file offset */
typedef	oskit_uid_t	uid_t;		/* user id */
typedef	oskit_pid_t	pid_t;		/* process id */

#ifndef _TIME_T
#define	_TIME_T
typedef	oskit_time_t	time_t;
#endif


#endif	/* _OSKIT_C_SYS_TYPES_H_ */
