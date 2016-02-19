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
#ifndef _OSKIT_C_SYS_STAT_H_
#define	_OSKIT_C_SYS_STAT_H_

#include <oskit/compiler.h>
#include <oskit/time.h>

/*
 * This stat structure definition is identical
 * to the oskit_stat structure defined in <oskit/io/posixio.h>;
 * pointers can be freely cast between the two.
 */
struct stat {
	oskit_dev_t	st_dev;
	oskit_ino_t	st_ino;
	oskit_mode_t	st_mode;
	oskit_nlink_t	st_nlink;
	oskit_uid_t	st_uid;
	oskit_gid_t	st_gid;
	oskit_dev_t	st_rdev;
	oskit_timespec_t	st_atimespec;
	oskit_timespec_t	st_mtimespec;
	oskit_timespec_t	st_ctimespec;
	oskit_off_t	st_size;
	oskit_u64_t	st_blocks;
	oskit_u32_t	st_blksize;
};
#define	st_atime	st_atimespec.tv_sec
#define	st_mtime	st_mtimespec.tv_sec
#define	st_ctime	st_ctimespec.tv_sec

#include <oskit/io/posixio.h>

#define S_ISUID		OSKIT_S_ISUID
#define S_ISGID		OSKIT_S_ISGID
#define S_ISVTX		OSKIT_S_ISVTX
#define S_ISTXT		OSKIT_S_ISVTX

#define S_IRWXU		OSKIT_S_IRWXU
#define S_IRUSR		OSKIT_S_IRUSR
#define S_IWUSR		OSKIT_S_IWUSR
#define S_IXUSR		OSKIT_S_IXUSR
#define S_IRWXG		OSKIT_S_IRWXG
#define S_IRGRP		OSKIT_S_IRGRP
#define S_IWGRP		OSKIT_S_IWGRP
#define S_IXGRP		OSKIT_S_IXGRP
#define S_IRWXO		OSKIT_S_IRWXO
#define S_IROTH		OSKIT_S_IROTH
#define S_IWOTH		OSKIT_S_IWOTH
#define S_IXOTH		OSKIT_S_IXOTH
#define S_IFMT		OSKIT_S_IFMT
#define S_IFIFO		OSKIT_S_IFIFO
#define S_IFCHR		OSKIT_S_IFCHR
#define S_IFDIR		OSKIT_S_IFDIR
#define S_IFBLK		OSKIT_S_IFBLK
#define S_IFREG		OSKIT_S_IFREG
#define S_IFLNK		OSKIT_S_IFLNK

#ifndef _POSIX_SOURCE
#define	S_IREAD		S_IRUSR
#define	S_IWRITE	S_IWUSR
#define	S_IEXEC		S_IXUSR
#endif

#define S_ISDIR(m)	OSKIT_S_ISDIR(m)
#define S_ISCHR(m)	OSKIT_S_ISCHR(m)
#define S_ISBLK(m)	OSKIT_S_ISBLK(m)
#define S_ISREG(m)	OSKIT_S_ISREG(m)
#define S_ISFIFO(m)	OSKIT_S_ISFIFO(m)
#ifndef _POSIX_SOURCE
#define S_ISLNK(m)	OSKIT_S_ISLNK(m)
#endif

OSKIT_BEGIN_DECLS

int chmod(const char *path, oskit_mode_t mode);
int fchmod(int fd, oskit_mode_t mode);
int fstat(int fd, struct stat *buf);
int mkdir(const char *path, oskit_mode_t mode);
int mkfifo(const char *path, oskit_mode_t mode);
int mknod(const char *path, oskit_mode_t mode, oskit_dev_t dev);
int stat(const char *path, struct stat *buf);
int lstat(const char *path, struct stat *buf);
oskit_mode_t umask(oskit_mode_t cmask);

OSKIT_END_DECLS

#endif /* _OSKIT_C_SYS_STAT_H_ */
