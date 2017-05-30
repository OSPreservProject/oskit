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

#include <sys/errno.h>

#include <oskit/error.h>
#include <oskit/debug.h>
#include <oskit/dev/freebsd.h>

/* 
 * Map FreeBSD errno values to OSKit error codes.
 */
static oskit_error_t errtab[ELAST] =
{
    OSKIT_EPERM,
    OSKIT_ENOENT,
    OSKIT_ESRCH,
    OSKIT_EINTR,
    OSKIT_EIO,
    OSKIT_ENXIO,
    OSKIT_E2BIG,
    OSKIT_ENOEXEC,
    OSKIT_EBADF,
    OSKIT_ECHILD,
    OSKIT_EDEADLK,
    OSKIT_ENOMEM,
    OSKIT_EACCES,
    OSKIT_EFAULT,
    OSKIT_EINVAL, /* ENOTBLK */
    OSKIT_EBUSY,
    OSKIT_EEXIST,
    OSKIT_EXDEV,
    OSKIT_ENODEV,
    OSKIT_ENOTDIR,
    OSKIT_EISDIR,
    OSKIT_EINVAL,
    OSKIT_ENFILE,
    OSKIT_EMFILE,
    OSKIT_ENOTTY,
    OSKIT_ETXTBSY,
    OSKIT_EFBIG,
    OSKIT_ENOSPC,
    OSKIT_ESPIPE,
    OSKIT_EROFS,
    OSKIT_EMLINK,
    OSKIT_EPIPE,
    OSKIT_EDOM,
    OSKIT_ERANGE,
    OSKIT_EAGAIN,
    OSKIT_EINPROGRESS,
    OSKIT_EALREADY,
    OSKIT_ENOTSOCK,
    OSKIT_EDESTADDRREQ,
    OSKIT_EMSGSIZE,
    OSKIT_EPROTOTYPE,
    OSKIT_ENOPROTOOPT,
    OSKIT_EPROTONOSUPPORT,
    OSKIT_E_UNEXPECTED, /* ESOCKTNOSUPPORT */ 
    OSKIT_EOPNOTSUPP,
    OSKIT_E_UNEXPECTED, /* EPFNOSUPPORT */
    OSKIT_EAFNOSUPPORT,
    OSKIT_EADDRINUSE,
    OSKIT_EADDRNOTAVAIL,
    OSKIT_ENETDOWN,
    OSKIT_ENETUNREACH,
    OSKIT_E_UNEXPECTED, /* ENETRESET */
    OSKIT_ECONNABORTED,
    OSKIT_ECONNRESET,
    OSKIT_ENOBUFS,
    OSKIT_EISCONN,
    OSKIT_ENOTCONN,
    OSKIT_E_UNEXPECTED, /* ESHUTDOWN */
    OSKIT_E_UNEXPECTED, /* ETOOMANYREFS */
    OSKIT_ETIMEDOUT,
    OSKIT_ECONNREFUSED,
    OSKIT_ELOOP,
    OSKIT_ENAMETOOLONG,
    OSKIT_E_UNEXPECTED, /* EHOSTDOWN */
    OSKIT_EHOSTUNREACH,
    OSKIT_ENOTEMPTY,
    OSKIT_E_UNEXPECTED, /* EPROCLIM */
    OSKIT_E_UNEXPECTED, /* EUSERS  */
    OSKIT_EDQUOT,
    OSKIT_ESTALE,
    OSKIT_E_UNEXPECTED, /* EREMOTE */
    OSKIT_E_UNEXPECTED, /* EBADRPC */
    OSKIT_E_UNEXPECTED, /* ERPCMISMATCH */
    OSKIT_E_UNEXPECTED, /* EPROGUNAVAIL */
    OSKIT_E_UNEXPECTED, /* EPROGMISMATCH */
    OSKIT_E_UNEXPECTED, /* EPROCUNAVAIL */
    OSKIT_ENOLCK,
    OSKIT_ENOSYS,
    OSKIT_E_UNEXPECTED, /* EFTYPE */
    OSKIT_E_UNEXPECTED, /* EAUTH */
    OSKIT_E_UNEXPECTED, /* ENEEDAUTH */
    OSKIT_EIDRM,
    OSKIT_ENOMSG,
    OSKIT_EOVERFLOW,
    OSKIT_ECANCELED,
    OSKIT_EILSEQ,
};

oskit_error_t oskit_freebsd_xlate_errno(int freebsd_error)
{
	if (sizeof(errtab)/sizeof(errtab[0]) != ELAST)
		panic("fdev_freebsd: errtab out ouf sync");

	if (freebsd_error <= 0 || freebsd_error > ELAST)
		return OSKIT_E_UNEXPECTED;

	return errtab[freebsd_error-1];
}

