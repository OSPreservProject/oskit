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
 * Definition of the COM-based file system node interface,
 * which represents a VFS-style file system object.
 * This interface does not imply any per-open state,
 * although it can also be exported by per-open objects.
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
 * Finally, features that only apply at the per-open level
 * (i.e., at the level of file descriptors and open file descriptions)
 * and not at the lower per-object level are not included,
 * since these are per-object rather than per-open interfaces.
 */
#ifndef _OSKIT_FS_FILE_H_
#define _OSKIT_FS_FILE_H_

#include <oskit/com.h>
#include <oskit/io/posixio.h>

struct oskit_filesystem;
struct oskit_openfile;


/*
 * Access mode parameter passed to oskit_file->access().
 */
typedef oskit_u32_t oskit_amode_t;

#define OSKIT_F_OK	0x00		/* Existence test */
#define OSKIT_X_OK	0x01		/* Test for execute or search perms */
#define OSKIT_W_OK	0x02		/* Test for write permission */
#define OSKIT_R_OK	0x04		/* Test for read permission */


/*
 * File open flags for use in oskit_node->open().
 * Note that these flag definitions don't correspond to BSD or another OS.
 */
typedef oskit_u32_t	oskit_oflags_t;	/* File open flags */

/* POSIX.1-1990 */
#define OSKIT_O_ACCMODE	0x0003
#define OSKIT_O_RDONLY	0x0001
#define OSKIT_O_WRONLY	0x0002
#define OSKIT_O_RDWR	0x0003

#define OSKIT_O_CREAT	0x0004
#define OSKIT_O_EXCL	0x0008
#define OSKIT_O_NOCTTY	0x0010
#define OSKIT_O_TRUNC	0x0020
#define OSKIT_O_APPEND	0x0040
#define OSKIT_O_NONBLOCK	0x0080

/* POSIX.1-1993 */
#define OSKIT_O_DSYNC	0x0100
#define OSKIT_O_RSYNC	0x0200
#define OSKIT_O_SYNC	0x0400


/*
 * Basic file object interface,
 * IID 4aa7df94-7c74-11cf-b500-08000953adc2.
 * This interface corresponds directly to the set of POSIX/Unix operations
 * that can be done on objects in a file system.
 */
struct oskit_file {
	struct oskit_file_ops *ops;
};
typedef struct oskit_file oskit_file_t;

struct oskit_file_ops {

	/*** COM-specified IUnknown interface operations ***/
	OSKIT_COMDECL	(*query)(oskit_file_t *f,
				 const struct oskit_guid *iid,
				 void **out_ihandle);
	OSKIT_COMDECL_U	(*addref)(oskit_file_t *f);
	OSKIT_COMDECL_U	(*release)(oskit_file_t *f);


	/*** Operations inherited from oskit_posixio_t ***/
	OSKIT_COMDECL	(*stat)(oskit_file_t *f,
				oskit_stat_t *out_stats);
	OSKIT_COMDECL	(*setstat)(oskit_file_t *f, oskit_u32_t mask,
				   const oskit_stat_t *stats);
	OSKIT_COMDECL	(*pathconf)(oskit_file_t *f, oskit_s32_t option,
				    oskit_s32_t *out_val);


	/*** Operations specific to the oskit_file interface ***/

	/*
	 * Synchronize the state of this file with the underlying media.
	 * sync() synchronizes both the data and its metadata;
	 * datasync() only guarantees that the data itself is synchronized.
	 */
	OSKIT_COMDECL	(*sync)(oskit_file_t *f, oskit_bool_t wait);
	OSKIT_COMDECL	(*datasync)(oskit_file_t *f, oskit_bool_t wait);

	/*
	 * Attempt the types of access specified by the mask
	 * (one or more of R_OK, W_OK, and/or X_OK),
	 * and return any error that actually attempting such an access
	 * would have caused for the sender at the time of the 'access' call.
	 */
	OSKIT_COMDECL	(*access)(oskit_file_t *f, oskit_amode_t mask);

	/*
	 * Read the contents of a symbolic link,
	 * as an ASCII character string.
	 *
	 * If the file is not a symbolic link, then OSKIT_E_NOTIMPL
	 * is returned.
	 */
	OSKIT_COMDECL 	(*readlink)(oskit_file_t *d, 
				    char *buf, oskit_u32_t len, 
				    oskit_u32_t *out_actual);

	/*
	 * Attempt to open this file system object with the specified flags.
	 *
	 * If OSKIT_O_TRUNC is specified, then the file will be truncated
	 * to zero length. 
	 *
	 * This method may only be used on regular files and directories.
	 * Directories may not be opened with OSKIT_O_WRONLY, OSKIT_O_RDWR 
	 * or OSKIT_O_TRUNC.
	 *
	 * The file system may return success but with *out_openfile = NULL;
	 * this means the requested operation is allowed
	 * but the file system does not implement per-open state;
	 * the client or an intermediary must provide this functionality.
	 */
	OSKIT_COMDECL	(*open)(oskit_file_t *f, oskit_oflags_t flags,
				struct oskit_openfile **out_openfile);

	/*
	 * Obtain the file system in which this file resides.
	 */
	OSKIT_COMDECL	(*getfs)(oskit_file_t *f, 
				 struct oskit_filesystem **out_fs);

#if 0
	/*
	 * Determine the location on the underlying media
	 * at which a particular block or extent in this file is located.
	 * The requested file offset and extent size
	 * must be a multiple of the file system's block size.
	 * The returned size may be smaller than the requested size,
	 * typically because the file is disjoint on the underlying media.
	 * After a successful return,
	 * the extent will be "locked" in this location on disk
	 * until a corresponding call to bunmap() is made,
	 * or until the file is deleted or truncated
	 * so that this extent no longer exists in this file.
	 * Support for this method is optional;
	 * it should only be implemented if it can be done efficiently.
	 */
	OSKIT_COMDECL	(*bmap)(oskit_file_t *f, oskit_off_t file_ofs,
				oskit_off_t *out_disk_ofs,
				oskit_size_t *inout_size);

	/*
	 * Unmap (unlock) a block or extent previously mapped using bmap().
	 * The file_ofs must be exactly as specified to bmap(),
	 * and the disk_ofs and size must be exactly as returned  by bmap().
	 */
	OSKIT_COMDECL	(*bunmap)(oskit_file_t *f, oskit_off_t file_ofs,
				  oskit_off_t disk_ofs, oskit_size_t size);
#endif

#if 0
	/*
	 * Support for general buffer-based file I/O
	 * including scatter/gather and cross-address-space data transfer.
	 * For these operations, the requested offset and amount
	 * does _not_ have to be a multiple of the file system's block size.
	 * The data will be read or written to/from
	 * the specified abstract buffer object.
	 * Support for these methods is optional;
	 * they should only be implemented if they can be done efficiently.
	 */
	OSKIT_COMDECL	(*readbuf)(oskit_file_t *f, oskit_off_t offset,
				   oskit_size_t amount, bufio_t *buf,
				   oskit_size_t *out_actual);
	OSKIT_COMDECL	(*writebuf)(oskit_file_t *f, oskit_off_t offset,
				    oskit_size_t amount, bufio_t *buf,
				    oskit_size_t *out_actual);
#endif
};

#define oskit_file_query(f, iid, out_ihandle) \
	((f)->ops->query((oskit_file_t *)(f), (iid), (out_ihandle)))
#define oskit_file_addref(f) \
	((f)->ops->addref((oskit_file_t *)(f)))
#define oskit_file_release(f) \
	((f)->ops->release((oskit_file_t *)(f)))
#define oskit_file_stat(f, out_stats) \
	((f)->ops->stat((oskit_file_t *)(f), (out_stats)))
#define oskit_file_setstat(f, mask, stats) \
	((f)->ops->setstat((oskit_file_t *)(f), (mask), (stats)))
#define oskit_file_pathconf(f, option, out_val) \
	((f)->ops->pathconf((oskit_file_t *)(f), (option), (out_val)))
#define oskit_file_sync(f, wait) \
	((f)->ops->sync((oskit_file_t *)(f), (wait)))
#define oskit_file_datasync(f, wait) \
	((f)->ops->datasync((oskit_file_t *)(f), (wait)))
#define oskit_file_access(f, mask) \
	((f)->ops->access((oskit_file_t *)(f), (mask)))
#define oskit_file_readlink(d, buf, len, out_actual) \
	((d)->ops->readlink((oskit_file_t *)(d), (buf), (len), (out_actual)))
#define oskit_file_open(f, flags, out_openfile) \
	((f)->ops->open((oskit_file_t *)(f), (flags), (out_openfile)))
#define oskit_file_getfs(f, out_fs) \
	((f)->ops->getfs((oskit_file_t *)(f), (out_fs)))

/* GUID for oskit_file interface */
extern const struct oskit_guid oskit_file_iid;
#define OSKIT_FILE_IID OSKIT_GUID(0x4aa7df94, 0x7c74, 0x11cf, \
				0xb5, 0x00, 0x08, 0x00, 0x09, 0x53, 0xad, 0xc2)


#endif /* _OSKIT_FS_FILE_H_ */
