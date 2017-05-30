/*
 * Copyright (c) 1994-1999 University of Utah and the Flux Group.
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
#ifndef _OSKIT_C_FCNTL_H_
#define _OSKIT_C_FCNTL_H_

#include <oskit/compiler.h>

/* this has the definitions for OSKIT_O_RDONLY etc. - these might
 * move in the oskit/com/ directory eventually
 */
#include <oskit/fs/file.h>

/*
 * These values are the same as those used in the OSKit file system interfaces.
 * Note that BSD, Linux, and other Unixoids use different values.
 * In particular, our O_RDONLY and O_WRONLY values are single bits
 * that, when OR'd together, produce O_RDWR;
 * be aware that this is not the case in traditional Unices.
 */
#define O_ACCMODE	(OSKIT_O_RDONLY | OSKIT_O_WRONLY | OSKIT_O_RDWR)
#define O_RDONLY	OSKIT_O_RDONLY
#define O_WRONLY	OSKIT_O_WRONLY
#define O_RDWR		OSKIT_O_RDWR

#define O_NONBLOCK	OSKIT_O_NONBLOCK
#define O_APPEND	OSKIT_O_APPEND
#define O_CREAT		OSKIT_O_CREAT
#define O_TRUNC		OSKIT_O_TRUNC
#define O_EXCL		OSKIT_O_EXCL
#define	O_NOCTTY	OSKIT_O_NOCTTY

/*
 * Constants used for fcntl(2)
 */

/* command values */
#define	F_DUPFD		0		/* duplicate file descriptor */
#define	F_GETFD		1		/* get file descriptor flags */
#define	F_SETFD		2		/* set file descriptor flags */
#define	F_GETFL		3		/* get file status flags */
#define	F_SETFL		4		/* set file status flags */
#ifndef _POSIX_SOURCE	/* XXX should these be here? */
#define	F_GETOWN	5		/* get SIGIO/SIGURG proc/pgrp */
#define F_SETOWN	6		/* set SIGIO/SIGURG proc/pgrp */
#endif
#define	F_GETLK		7		/* get record locking information */
#define	F_SETLK		8		/* set record locking information */
#define	F_SETLKW	9		/* F_SETLK; wait if blocked */

/* file descriptor flags (F_GETFD, F_SETFD) */
#define	FD_CLOEXEC	1		/* close-on-exec flag */


OSKIT_BEGIN_DECLS

int fcntl(int fd, int cmd, ...);
int creat(const char *__name, oskit_mode_t __mode);
int open(const char *__name, int __mode, ...);

OSKIT_END_DECLS

#endif /* _OSKIT_C_FCNTL_H_ */
