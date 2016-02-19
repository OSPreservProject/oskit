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
/*
 * Definition of the minimal COM-based POSIX I/O interface,
 * which is the minimal interface any POSIX I/O object
 * (file, device, pipe, socket, etc.) can be expected to support.
 * Basically, this consists of the standard information query
 * and modification functionality: stat, pathconf,
 * and the various functions that modify an object's stat elements;
 * these modification functions are rolled into one 'setstat' method.
 * Of course, not all object types will return sensible values
 * for all stat elements, or allow them to to be modified.
 *
 * In general, these interfaces only include features
 * that are supported in well-known existing standards:
 * in, particular, the POSIX and the Unix standards.
 * Orthogonal non-base functionality
 * is generally separated out into separate interfaces
 * in order to keep the basic mandatory interfaces minimal
 * (e.g., tty interfaces, STREAMS, async I/O, etc.).
 * Also, functions from more restricted standards
 * that are subsumed by other functions in larger standards
 * have been omitted (e.g., utime versus utimes).
 */
#ifndef _OSKIT_IO_POSIXIO_H_
#define _OSKIT_IO_POSIXIO_H_

#include <oskit/com.h>
#include <oskit/types.h>
#include <oskit/time.h>

struct oskit_posixio;

/*
 * File mode (file type and access permission) definitions.
 * Clients must ignore bits that aren't defined here
 * in mode values received in oskit_stat structures.
 * These values happen to be the traditional Unix definitions.
 */

#define	OSKIT_S_ISUID	0004000		/* Set user id on execution */
#define	OSKIT_S_ISGID	0002000		/* Set group id on execution */
#define OSKIT_S_ISVTX	0001000		/* Sticky bit */

#define	OSKIT_S_IRWXU	0000700		/* RWX mask for owner */
#define	OSKIT_S_IRUSR	0000400		/* R for owner */
#define	OSKIT_S_IWUSR	0000200		/* W for owner */
#define	OSKIT_S_IXUSR	0000100		/* X for owner */

#define	OSKIT_S_IRWXG	0000070		/* RWX mask for group */
#define	OSKIT_S_IRGRP	0000040		/* R for group */
#define	OSKIT_S_IWGRP	0000020		/* W for group */
#define	OSKIT_S_IXGRP	0000010		/* X for group */

#define	OSKIT_S_IRWXO	0000007		/* RWX mask for other */
#define	OSKIT_S_IROTH	0000004		/* R for other */
#define	OSKIT_S_IWOTH	0000002		/* W for other */
#define	OSKIT_S_IXOTH	0000001		/* X for other */

#define	OSKIT_S_IFMT	0170000	/* Mask of file type bits, one of: */
#define	OSKIT_S_IFDIR	0040000	/* Directory */
#define	OSKIT_S_IFCHR	0020000	/* Char special */
#define	OSKIT_S_IFBLK	0060000	/* Block special */
#define	OSKIT_S_IFREG	0100000	/* Regular file */
#define	OSKIT_S_IFLNK	0120000	/* Symbolic link */
#define	OSKIT_S_IFSOCK	0140000	/* Local-domain socket */
#define	OSKIT_S_IFIFO	0010000	/* FIFO */

/* XXX these could be handled with COM querying */
#define	OSKIT_S_ISDIR(m)	(((m) & OSKIT_S_IFMT) == OSKIT_S_IFDIR)
#define	OSKIT_S_ISCHR(m)	(((m) & OSKIT_S_IFMT) == OSKIT_S_IFCHR)
#define	OSKIT_S_ISBLK(m)	(((m) & OSKIT_S_IFMT) == OSKIT_S_IFBLK)
#define	OSKIT_S_ISREG(m)	(((m) & OSKIT_S_IFMT) == OSKIT_S_IFREG)
#define	OSKIT_S_ISLNK(m)	(((m) & OSKIT_S_IFMT) == OSKIT_S_IFLNK)
#define	OSKIT_S_ISSOCK(m)(((m) & OSKIT_S_IFMT) == OSKIT_S_IFSOCK)
#define	OSKIT_S_ISFIFO(m) (((m) & OSKIT_S_IFMT) == OSKIT_S_IFIFO)

/*
 * stat structure definition
 */
struct oskit_stat {
	oskit_dev_t	dev;
	oskit_ino_t	ino;
	oskit_mode_t	mode;
	oskit_nlink_t	nlink;
	oskit_uid_t	uid;
	oskit_gid_t	gid;
	oskit_dev_t	rdev;
	oskit_timespec_t	atime;
	oskit_timespec_t	mtime;
	oskit_timespec_t	ctime;
	oskit_off_t	size;
	oskit_u64_t	blocks;
	oskit_u32_t	blksize;
};
typedef struct oskit_stat oskit_stat_t;

/*
 * Masks to setstat or stat.
 */
#define OSKIT_STAT_DEV		0x0001 /* not settable */
#define OSKIT_STAT_INO		0x0002 /* not settable */
#define OSKIT_STAT_MODE		0x0004 /* setting file type bits ignored */
#define OSKIT_STAT_NLINK		0x0008 /* not settable */
#define OSKIT_STAT_UID		0x0010
#define OSKIT_STAT_GID		0x0020
#define OSKIT_STAT_RDEV		0x0040 /* not settable */
#define OSKIT_STAT_ATIME		0x0080
#define OSKIT_STAT_MTIME		0x0100
#define OSKIT_STAT_UTIMES_NULL	0x0200 /* set atime & mtime to current time */
#define OSKIT_STAT_CTIME		0x0400 /* not settable */
#define OSKIT_STAT_SIZE		0x0800
#define OSKIT_STAT_BLOCKS	0x1000 /* not settable */
#define OSKIT_STAT_BLKSIZE	0x2000 /* not settable */
#define OSKIT_STAT_SETTABLE	(OSKIT_STAT_MODE | OSKIT_STAT_UID | OSKIT_STAT_GID | OSKIT_STAT_ATIME | OSKIT_STAT_MTIME | OSKIT_STAT_UTIMES_NULL | OSKIT_STAT_SIZE)


/*
 * Configurable options that can be requested via oskit_node->pathconf().
 */
#define OSKIT_PC_LINK_MAX	 1	/* Maximum file link count */
#define OSKIT_PC_MAX_CANON	 2	/* Max size of terminal input line */
#define OSKIT_PC_MAX_INPUT	 3	/* Max input queue size */
#define OSKIT_PC_NAME_MAX	 4	/* Max number of bytes in a filename */
#define OSKIT_PC_PATH_MAX	 5	/* Max number of bytes in a pathname */
#define OSKIT_PC_PIPE_BUF	 6	/* Max atomic write size to a pipe */
#define OSKIT_PC_CHOWN_RESTRICTED 7	/* Use of chown() is restricted */
#define OSKIT_PC_NO_TRUNC	 8	/* Too-long pathnames produce errors */
#define OSKIT_PC_VDISABLE	 9	/* Value to disable special term chrs */
/* POSIX.1b (realtime extensions) */
#define OSKIT_PC_ASYNC_IO	10	/* Asynchronous I/O supported */
#define OSKIT_PC_PRIO_IO		11	/* Prioritized I/O supported */
#define OSKIT_PC_SYNC_IO		12	/* Synchronized I/O supported */


/*
 * Basic open-file object interface,
 * IID 4aa7df8f-7c74-11cf-b500-08000953adc2.
 * This interface corresponds directly
 * to the set of POSIX.1 operations
 * that can be done on ordinary file descriptors.
 * This interface represents the base type for all POSIX file system nodes,
 * including regular files, directories, etc.,
 * in addition to non-file system objects such as pipes and sockets.
 */
struct oskit_posixio {
	struct oskit_posixio_ops *ops;
};
typedef struct oskit_posixio oskit_posixio_t;

struct oskit_posixio_ops {

	/*** COM-specified IUnknown interface operations ***/
	OSKIT_COMDECL	(*query)(oskit_posixio_t *f,
				 const struct oskit_guid *iid,
				 void **out_ihandle);
	OSKIT_COMDECL_U	(*addref)(oskit_posixio_t *f);
	OSKIT_COMDECL_U	(*release)(oskit_posixio_t *f);

	/*** Operations derived from POSIX.1-1990 ***/

	/* Return statistics about this node */
	OSKIT_COMDECL	(*stat)(oskit_posixio_t *f, oskit_stat_t *out_stats);

	/* Set some or all of the modifiable statistics in this node */
	OSKIT_COMDECL	(*setstat)(oskit_posixio_t *f, oskit_u32_t mask,
				   const oskit_stat_t *stat);

	/* Determine configuration information about a file */
	OSKIT_COMDECL	(*pathconf)(oskit_posixio_t *f, oskit_s32_t option,
				    oskit_s32_t *out_val);
};


#define oskit_posixio_query(f, iid, out_ihandle) \
	((f)->ops->query((oskit_posixio_t *)(f), (iid), (out_ihandle)))
#define oskit_posixio_addref(f) \
	((f)->ops->addref((oskit_posixio_t *)(f)))
#define oskit_posixio_release(f) \
	((f)->ops->release((oskit_posixio_t *)(f)))
#define oskit_posixio_stat(f, out_stats) \
	((f)->ops->stat((oskit_posixio_t *)(f), (out_stats)))
#define oskit_posixio_setstat(f, mask, stat) \
	((f)->ops->setstat((oskit_posixio_t *)(f), (mask), (stat)))
#define oskit_posixio_pathconf(f, option, out_val) \
	((f)->ops->pathconf((oskit_posixio_t *)(f), (option), (out_val)))


/* GUID for oskit_posixio interface */
extern const struct oskit_guid oskit_posixio_iid;
#define OSKIT_POSIXIO_IID OSKIT_GUID(0x4aa7df8f, 0x7c74, 0x11cf, \
				0xb5, 0x00, 0x08, 0x00, 0x09, 0x53, 0xad, 0xc2)


#endif /* _OSKIT_IO_POSIXIO_H_ */
