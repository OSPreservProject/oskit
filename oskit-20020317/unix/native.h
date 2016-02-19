/*
 * Copyright (c) 1999, 2000, 2002 University of Utah and the Flux Group.
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

#define NATIVEOS(x)	native_##x
#define NATIVEDECL(x)	extern __typeof(x) native_##x

#include <oskit/compiler.h>
#include <oskit/c/assert.h>
void panic(const char *__format, ...) OSKIT_NORETURN; /* XXX */

#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/sysctl.h>
#include <time.h>
#include <unistd.h>
#include <dirent.h>
#include <signal.h>

NATIVEDECL(_exit);
NATIVEDECL(accept);
NATIVEDECL(access);
NATIVEDECL(bind);
NATIVEDECL(chmod);
NATIVEDECL(chown);
NATIVEDECL(close);
NATIVEDECL(connect);
NATIVEDECL(fchdir);
NATIVEDECL(fcntl);
NATIVEDECL(fsync);
NATIVEDECL(getdirentries);
NATIVEDECL(gethostname);
NATIVEDECL(getpeername);
NATIVEDECL(getpid);
NATIVEDECL(getsockname);
NATIVEDECL(getsockopt);
NATIVEDECL(gettimeofday);
NATIVEDECL(ioctl);
NATIVEDECL(kill);
NATIVEDECL(link);
NATIVEDECL(listen);
NATIVEDECL(lseek);
NATIVEDECL(lstat);
NATIVEDECL(mkdir);
NATIVEDECL(mmap);
NATIVEDECL(mprotect);
NATIVEDECL(munmap);
NATIVEDECL(open);
NATIVEDECL(profil);
NATIVEDECL(read);
NATIVEDECL(readlink);
NATIVEDECL(recvfrom);
NATIVEDECL(rename);
NATIVEDECL(rmdir);
NATIVEDECL(sbrk);
NATIVEDECL(select);
NATIVEDECL(sendto);
NATIVEDECL(setitimer);
NATIVEDECL(setsockopt);
NATIVEDECL(settimeofday);
NATIVEDECL(shutdown);
NATIVEDECL(sigaction);
NATIVEDECL(sigaddset);
NATIVEDECL(sigaltstack);
NATIVEDECL(sigdelset);
NATIVEDECL(sigemptyset);
NATIVEDECL(sigfillset);
NATIVEDECL(sigismember);
NATIVEDECL(sigprocmask);
NATIVEDECL(socket);
NATIVEDECL(socketpair);
NATIVEDECL(stat);
NATIVEDECL(symlink);
NATIVEDECL(sysctl);
NATIVEDECL(truncate);
NATIVEDECL(unlink);
NATIVEDECL(utimes);
NATIVEDECL(write);

#ifdef NATIVE_HAVE_D_NAMLEN
#define DIRENTNAMLEN(d) ((d)->d_namlen)
#else
#define DIRENTNAMLEN(d) (strlen ((d)->d_name))
#endif
#ifdef NATIVE_HAVE_D_INO
#define DIRENTINO(d)	((d)->d_ino)
#elif defined NATIVE_HAVE_D_FILENO
#define DIRENTINO(d)	((d)->d_fileno)
#else
#define DIRENTINO(d)	(0)
#endif

#ifdef NATIVE_HAVE_ST_ATIMESPEC
#define	STATATIMESEC(st)	((st).st_atimespec.tv_sec)
#define	STATATIMENSEC(st)	((st).st_atimespec.tv_nsec)
#define	STATMTIMESEC(st)	((st).st_mtimespec.tv_sec)
#define	STATMTIMENSEC(st)	((st).st_mtimespec.tv_nsec)
#define	STATCTIMESEC(st)	((st).st_ctimespec.tv_sec)
#define	STATCTIMENSEC(st)	((st).st_ctimespec.tv_nsec)
#else
#define	STATATIMESEC(st)	((st).st_atime)
#define	STATATIMENSEC(st)	((void) (st), 0)
#define	STATMTIMESEC(st)	((st).st_mtime)
#define	STATMTIMENSEC(st)	((void) (st), 0)
#define	STATCTIMESEC(st)	((st).st_ctime)
#define	STATCTIMENSEC(st)	((void) (st), 0)
#endif

#include "native_errno.h"
