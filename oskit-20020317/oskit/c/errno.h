/*
 * Copyright (c) 1995-1998 University of Utah and the Flux Group.
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
 * This header file defines a set of POSIX errno values
 * that fits consistently into the COM error code namespace -
 * i.e. these error code values can be returned from COM object methods.
 * This header file just reuses the values defined in <oskit/error.h>,
 * so that programs using the oskit's minimal C library
 * can treat errno values and interface return codes interchangeably.
 *
 * One (probably the main) disadvantage of using these error codes
 * is that, since they don't start from around 0 like typical Unix errno values,
 * it's impossible to provide a Unix-style sys_errlist table for them.
 * However, they are fully compatible
 * with the POSIX-blessed strerror and perror routines,
 * and in any case the minimal C library
 * is not intended to support "legacy" applications directly -
 * for that purpose, a "real" C library would be more appropriate,
 * and such a C library would probably use more traditional errno values,
 * doing appropriate translation when interacting with COM interfaces.
 */
#ifndef _OSKIT_C_ERRNO_H_
#define _OSKIT_C_ERRNO_H_

#include <oskit/error.h>

#if !defined(errno) && !defined(ASSEMBLER)
extern int errno;			/* global error number */
#endif

/* ISO/ANSI C-1990 errors */
#define	EDOM		OSKIT_EDOM	/* Numerical argument out of domain */
#define	ERANGE		OSKIT_ERANGE	/* Result too large */

/* POSIX-1990 errors */
#define	E2BIG		OSKIT_E2BIG	/* Argument list too long */
#define	EACCES		OSKIT_EACCES	/* Permission denied */
#define	EAGAIN		OSKIT_EAGAIN	/* Resource temporarily unavailable */
#define	EBADF		OSKIT_EBADF	/* Bad file descriptor */
#define	EBUSY		OSKIT_EBUSY	/* Device busy */
#define	ECHILD		OSKIT_ECHILD	/* No child processes */
#define	EDEADLK		OSKIT_EDEADLK	/* Resource deadlock avoided */
#define	EEXIST		OSKIT_EEXIST	/* File exists */
#define	EFAULT		OSKIT_EFAULT	/* Bad address */
#define	EFBIG		OSKIT_EFBIG	/* File too large */
#define	EINTR		OSKIT_EINTR	/* Interrupted system call */
#define	EINVAL		OSKIT_EINVAL	/* Invalid argument */
#define	EIO		OSKIT_EIO	/* Input/output error */
#define	EISDIR		OSKIT_EISDIR	/* Is a directory */
#define	EMFILE		OSKIT_EMFILE	/* Too many open files */
#define	EMLINK		OSKIT_EMLINK	/* Too many links */
#define	ENAMETOOLONG	OSKIT_ENAMETOOLONG /* File name too long */
#define	ENFILE		OSKIT_ENFILE	/* Too many open files in system */
#define	ENODEV		OSKIT_ENODEV	/* Operation not supported by device */
#define	ENOENT		OSKIT_ENOENT	/* No such file or directory */
#define	ENOEXEC		OSKIT_ENOEXEC	/* Exec format error */
#define	ENOLCK		OSKIT_ENOLCK	/* No locks available */
#define	ENOMEM		OSKIT_ENOMEM	/* Cannot allocate memory */
#define	ENOSPC		OSKIT_ENOSPC	/* No space left on device */
#define	ENOSYS		OSKIT_ENOSYS	/* Function not implemented */
#define	ENOTDIR		OSKIT_ENOTDIR	/* Not a directory */
#define	ENOTEMPTY	OSKIT_ENOTEMPTY	/* Directory not empty */
#define	ENOTTY		OSKIT_ENOTTY	/* Inappropriate ioctl for device */
#define	ENXIO		OSKIT_ENXIO	/* Device not configured */
#define	EPERM		OSKIT_EPERM	/* Operation not permitted */
#define	EPIPE		OSKIT_EPIPE	/* Broken pipe */
#define	EROFS		OSKIT_EROFS	/* Read-only file system */
#define	ESPIPE		OSKIT_ESPIPE	/* Illegal seek */
#define	ESRCH		OSKIT_ESRCH	/* No such process */
#define	EXDEV		OSKIT_EXDEV	/* Cross-device link */

/* POSIX-1993 errors */
#define EBADMSG		OSKIT_EBADMSG	/* Bad message */
#define ECANCELED	OSKIT_ECANCELED	/* Operation canceled */
#define EINPROGRESS	OSKIT_EINPROGRESS /* Operation in progress */
#define EMSGSIZE	OSKIT_EMSGSIZE	/* Bad message buffer length */
#define ENOTSUP		OSKIT_ENOTSUP	/* Not supported */

/* POSIX-1996 errors */
#define ETIMEDOUT	OSKIT_ETIMEDOUT	/* Operation timed out */

/* X/Open UNIX 1994 errors */
#define EADDRINUSE	OSKIT_EADDRINUSE		/* Address in use */
#define EADDRNOTAVAIL	OSKIT_EADDRNOTAVAIL	/* Address not available */
#define EAFNOSUPPORT	OSKIT_EAFNOSUPPORT	/* Address family unsupported */
#define EALREADY	OSKIT_EALREADY		/* Already connected */
#define ECONNABORTED	OSKIT_ECONNABORTED	/* Connection aborted */
#define ECONNREFUSED	OSKIT_ECONNREFUSED	/* Connection refused */
#define ECONNRESET	OSKIT_ECONNRESET		/* Connection reset */
#define EDESTADDRREQ	OSKIT_EDESTADDRREQ	/* Dest address required */
#define EDQUOT		OSKIT_EDQUOT		/* Reserved */
#define EHOSTUNREACH	OSKIT_EHOSTUNREACH	/* Host is unreachable */
#define EIDRM		OSKIT_EIDRM		/* Identifier removed */
#define EILSEQ		OSKIT_EILSEQ		/* Illegal byte sequence */
#define EISCONN		OSKIT_EISCONN		/* Connection in progress */
#define ELOOP		OSKIT_ELOOP		/* Too many symbolic links */
#define EMULTIHOP	OSKIT_EMULTIHOP		/* Reserved */
#define ENETDOWN	OSKIT_ENETDOWN		/* Network is down */
#define ENETUNREACH	OSKIT_ENETUNREACH	/* Network unreachable */
#define ENOBUFS		OSKIT_ENOBUFS		/* No buffer space available */
#define ENODATA		OSKIT_ENODATA		/* No message is available */
#define ENOLINK		OSKIT_ENOLINK		/* Reserved */
#define ENOMSG		OSKIT_ENOMSG		/* No message of desired type */
#define ENOPROTOOPT	OSKIT_ENOPROTOOPT	/* Protocol not available */
#define ENOSR		OSKIT_ENOSR		/* No STREAM resources */
#define ENOSTR		OSKIT_ENOSTR		/* Not a STREAM */
#define ENOTCONN	OSKIT_ENOTCONN		/* Socket not connected */
#define ENOTSOCK	OSKIT_ENOTSOCK		/* Not a socket */
#define EOPNOTSUPP	OSKIT_EOPNOTSUPP		/* Op not supported on socket */
#define EOVERFLOW	OSKIT_EOVERFLOW		/* Value too large */
#define EPROTO		OSKIT_EPROTO		/* Protocol error */
#define EPROTONOSUPPORT	OSKIT_EPROTONOSUPPORT	/* Protocol not supported */
#define EPROTOTYPE	OSKIT_EPROTOTYPE		/* Socket typ not supported */
#define ESTALE		OSKIT_ESTALE		/* Reserved */
#define ETIME		OSKIT_ETIME		/* Stream ioctl timeout */
#define ETXTBSY		OSKIT_ETXTBSY		/* Text file busy */
#define EWOULDBLOCK	OSKIT_EWOULDBLOCK	/* Operation would block */

#endif /* _OSKIT_C_ERRNO_H_ */
