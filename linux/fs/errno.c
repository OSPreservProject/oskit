/*
 * Copyright (c) 1997, 1998 The University of Utah and the Flux Group.
 * 
 * This file is part of the OSKit Linux Glue Libraries, which are free
 * software, also known as "open source;" you can redistribute them and/or
 * modify them under the terms of the GNU General Public License (GPL),
 * version 2, as published by the Free Software Foundation (FSF).
 * 
 * The OSKit is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GPL for more details.  You should have
 * received a copy of the GPL along with the OSKit; see the file COPYING.  If
 * not, write to the FSF, 59 Temple Place #330, Boston, MA 02111-1307, USA.
 */
/*
 * Map Linux errno vals to OSKit error codes.
 */

#include "errno.h"

oskit_error_t linux_errno_to_oskit_error[EDQUOT] = {
	OSKIT_EPERM,			/*   1 = EPERM */
	OSKIT_ENOENT,			/*   2 = ENOENT */
	OSKIT_ESRCH,			/*   3 = ESRCH */
	OSKIT_EINTR,			/*   4 = EINTR */
	OSKIT_EIO,			/*   5 = EIO */
	OSKIT_ENXIO,			/*   6 = ENXIO */
	OSKIT_E2BIG,			/*   7 = E2BIG */
	OSKIT_ENOEXEC,			/*   8 = ENOEXEC */
	OSKIT_EBADF,			/*   9 = EBADF */
	OSKIT_ECHILD,			/*  10 = ECHILD */
	OSKIT_EAGAIN,			/*  11 = EAGAIN */
	OSKIT_ENOMEM,			/*  12 = ENOMEM */
	OSKIT_EACCES,			/*  13 = EACCES */
	OSKIT_EFAULT,			/*  14 = EFAULT */
/**/	OSKIT_EINVAL,			/*  15 = ENOTBLK */
	OSKIT_EBUSY,			/*  16 = EBUSY */
	OSKIT_EEXIST,			/*  17 = EEXIST */
	OSKIT_EXDEV,			/*  18 = EXDEV */
	OSKIT_ENODEV,			/*  19 = ENODEV */
	OSKIT_ENOTDIR,			/*  20 = ENOTDIR */
	OSKIT_EISDIR,			/*  21 = EISDIR */
	OSKIT_EINVAL,			/*  22 = EINVAL */
	OSKIT_ENFILE,			/*  23 = ENFILE */
	OSKIT_EMFILE,			/*  24 = EMFILE */
	OSKIT_ENOTTY,			/*  25 = ENOTTY */
	OSKIT_ETXTBSY,			/*  26 = ETXTBSY */
	OSKIT_EFBIG,			/*  27 = EFBIG */
	OSKIT_ENOSPC,			/*  28 = ENOSPC */
	OSKIT_ESPIPE,			/*  29 = ESPIPE */
	OSKIT_EROFS,			/*  30 = EROFS */
	OSKIT_EMLINK,			/*  31 = EMLINK */
	OSKIT_EPIPE,			/*  32 = EPIPE */
	OSKIT_EDOM,			/*  33 = EDOM */
	OSKIT_ERANGE,			/*  34 = ERANGE */
	OSKIT_EDEADLK,			/*  35 = EDEADLK */
	OSKIT_ENAMETOOLONG,		/*  36 = ENAMETOOLONG */
	OSKIT_ENOLCK,			/*  37 = ENOLCK */
	OSKIT_ENOSYS,			/*  38 = ENOSYS */
	OSKIT_ENOTEMPTY,		/*  39 = ENOTEMPTY */
	OSKIT_ELOOP,			/*  40 = ELOOP */
/**/	OSKIT_EINVAL,			/*  41 = unassigned */
	OSKIT_ENOMSG,			/*  42 = ENOMSG */
	OSKIT_EIDRM,			/*  43 = EIDRM */
/**/	OSKIT_EINVAL,			/*  44 = ECHRNG */
/**/	OSKIT_EINVAL,			/*  45 = EL2NSYNC */
/**/	OSKIT_EINVAL,			/*  46 = EL3HLT */
/**/	OSKIT_EINVAL,			/*  47 = EL3RST */
/**/	OSKIT_EINVAL,			/*  48 = ELNRNG */
/**/	OSKIT_EINVAL,			/*  49 = EUNATCH */
/**/	OSKIT_EINVAL,			/*  50 = ENOCSI */
/**/	OSKIT_EINVAL,			/*  51 = EL2HLT */
/**/	OSKIT_EINVAL,			/*  52 = EBADE */
/**/	OSKIT_EINVAL,			/*  53 = EBADR */
/**/	OSKIT_EINVAL,			/*  54 = EXFULL */
/**/	OSKIT_EINVAL,			/*  55 = ENOANO */
/**/	OSKIT_EINVAL,			/*  56 = EBADRQC */
/**/	OSKIT_EINVAL,			/*  57 = EBADSLT */
/**/	OSKIT_EINVAL,			/*  58 = unassigned */
/**/	OSKIT_EINVAL,			/*  59 = EBFONT */
	OSKIT_ENOSTR,			/*  60 = ENOSTR */
	OSKIT_ENODATA,			/*  61 = ENODATA */
	OSKIT_ETIME,			/*  62 = ETIME */
	OSKIT_ENOSR,			/*  63 = ENOSR */
/**/	OSKIT_EINVAL,			/*  64 = ENONET */
/**/	OSKIT_EINVAL,			/*  65 = ENOPKG */
/**/	OSKIT_EINVAL,			/*  66 = EREMOTE */
	OSKIT_ENOLINK,			/*  67 = ENOLINK */
/**/	OSKIT_EINVAL,			/*  68 = EADV */
/**/	OSKIT_EINVAL,			/*  69 = ESRMNT */
/**/	OSKIT_EINVAL,			/*  70 = ECOMM */
	OSKIT_EPROTO,			/*  71 = EPROTO */
	OSKIT_EMULTIHOP,		/*  72 = EMULTIHOP */
/**/	OSKIT_EINVAL,			/*  73 = EDOTDOT */
	OSKIT_EBADMSG,			/*  74 = EBADMSG */
	OSKIT_EOVERFLOW,		/*  75 = EOVERFLOW */
/**/	OSKIT_EINVAL,			/*  76 = ENOTUNIQ */
/**/	OSKIT_EINVAL,			/*  77 = EBADFD */
/**/	OSKIT_EINVAL,			/*  78 = EREMCHG */
/**/	OSKIT_EINVAL,			/*  79 = ELIBACC */
/**/	OSKIT_EINVAL,			/*  80 = ELIBBAD */
/**/	OSKIT_EINVAL,			/*  81 = ELIBSCN */
/**/	OSKIT_EINVAL,			/*  82 = ELIBMAX */
/**/	OSKIT_EINVAL,			/*  83 = ELIBEXEC */
	OSKIT_EILSEQ,			/*  84 = EILSEQ */
/**/	OSKIT_EINVAL,			/*  85 = ERESTART */
/**/	OSKIT_EINVAL,			/*  86 = ESTRPIPE */
/**/	OSKIT_EINVAL,			/*  87 = EUSERS */
	OSKIT_ENOTSOCK,			/*  88 = ENOTSOCK */
	OSKIT_EDESTADDRREQ,		/*  89 = EDESTADDRREQ */
	OSKIT_EMSGSIZE,			/*  90 = EMSGSIZE */
	OSKIT_EPROTOTYPE,		/*  91 = EPROTOTYPE */
	OSKIT_ENOPROTOOPT,		/*  92 = ENOPROTOOPT */
	OSKIT_EPROTONOSUPPORT,		/*  93 = EPROTONOSUPPORT */
/**/	OSKIT_EINVAL,			/*  94 = ESOCKTNOSUPPORT */
	OSKIT_EOPNOTSUPP,		/*  95 = EOPNOTSUPP */
/**/	OSKIT_EINVAL,			/*  96 = EPFNOSUPPORT */
	OSKIT_EAFNOSUPPORT,		/*  97 = EAFNOSUPPORT */
	OSKIT_EADDRINUSE,		/*  98 = EADDRINUSE */
	OSKIT_EADDRNOTAVAIL,		/*  99 = EADDRNOTAVAIL */
	OSKIT_ENETDOWN,			/* 100 = ENETDOWN */
	OSKIT_ENETUNREACH,		/* 101 = ENETUNREACH */
/**/	OSKIT_EINVAL,			/* 102 = ENETRESET */
	OSKIT_ECONNABORTED,		/* 103 = ECONNABORTED */
	OSKIT_ECONNRESET,		/* 104 = ECONNRESET */
	OSKIT_ENOBUFS,			/* 105 = ENOBUFS */
	OSKIT_EISCONN,			/* 106 = EISCONN */
	OSKIT_ENOTCONN,			/* 107 = ENOTCONN */
/**/	OSKIT_EINVAL,			/* 108 = ESHUTDOWN */
/**/	OSKIT_EINVAL,			/* 109 = ETOOMANYREFS */
	OSKIT_ETIMEDOUT,		/* 110 = ETIMEDOUT */
	OSKIT_ECONNREFUSED,		/* 111 = ECONNREFUSED */
/**/	OSKIT_EINVAL,			/* 112 = EHOSTDOWN */
	OSKIT_EHOSTUNREACH,		/* 113 = EHOSTUNREACH */
	OSKIT_EALREADY,			/* 114 = EALREADY */
	OSKIT_EINPROGRESS,		/* 115 = EINPROGRESS */
	OSKIT_ESTALE,			/* 116 = ESTALE */
/**/	OSKIT_EINVAL,			/* 117 = EUCLEAN */
/**/	OSKIT_EINVAL,			/* 118 = ENOTNAM */
/**/	OSKIT_EINVAL,			/* 119 = ENAVAIL */
/**/	OSKIT_EINVAL,			/* 120 = EISNAM */
/**/	OSKIT_EINVAL,			/* 121 = EREMOTEIO */
	OSKIT_EDQUOT,			/* 122 = EDQUOT */
};
