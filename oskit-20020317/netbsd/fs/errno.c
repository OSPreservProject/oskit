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

#include "errno.h"

/* 
 * Map NetBSD errno values to OSKit error codes.
 */

oskit_error_t netbsd_errno_to_oskit_error[ELAST] =
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
    OSKIT_EINVAL, /* ESOCKTNOSUPPORT */ 
    OSKIT_EOPNOTSUPP,
    OSKIT_EINVAL, /* EPFNOSUPPORT */
    OSKIT_EAFNOSUPPORT,
    OSKIT_EADDRINUSE,
    OSKIT_EADDRNOTAVAIL,
    OSKIT_ENETDOWN,
    OSKIT_ENETUNREACH,
    OSKIT_EINVAL, /* ENETRESET */
    OSKIT_ECONNABORTED,
    OSKIT_ECONNRESET,
    OSKIT_ENOBUFS,
    OSKIT_EISCONN,
    OSKIT_ENOTCONN,
    OSKIT_EINVAL, /* ESHUTDOWN */
    OSKIT_EINVAL, /* ETOOMANYREFS */
    OSKIT_ETIMEDOUT,
    OSKIT_ECONNREFUSED,
    OSKIT_ELOOP,
    OSKIT_ENAMETOOLONG,
    OSKIT_EINVAL, /* EHOSTDOWN */
    OSKIT_EHOSTUNREACH,
    OSKIT_ENOTEMPTY,
    OSKIT_EINVAL, /* EPROCLIM */
    OSKIT_EINVAL, /* EUSERS  */
    OSKIT_EDQUOT,
    OSKIT_ESTALE,
    OSKIT_EINVAL, /* EREMOTE */
    OSKIT_EINVAL, /* EBADRPC */
    OSKIT_EINVAL, /* ERPCMISMATCH */
    OSKIT_EINVAL, /* EPROGUNAVAIL */
    OSKIT_EINVAL, /* EPROGMISMATCH */
    OSKIT_EINVAL, /* EPROCUNAVAIL */
    OSKIT_ENOLCK,
    OSKIT_ENOSYS,
    OSKIT_EINVAL, /* EFTYPE */
    OSKIT_EINVAL, /* EAUTH */
    OSKIT_EINVAL  /* ENEEDAUTH */
};
