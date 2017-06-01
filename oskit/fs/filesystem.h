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
 * Definition of the COM file system interface,
 * which represents a file system as a whole.
 */
#ifndef _OSKIT_COM_FILESYSTEM_H_
#define _OSKIT_COM_FILESYSTEM_H_

#include <oskit/com.h>

/*
 * Result structure returned by oskit_filesystem->statfs().
 *
 * This is based on the statvfs definition in X/Open CAE 1994.
 *
 * Note that the f_ prefixes of the structure elements are omitted here
 * to avoid conflicts with #defines in an actual <sys/statvfs.h> header file,
 * since CAE explicitly allows those names to be #define's.
 */
struct oskit_statfs 
{	
	oskit_u32_t bsize;	/* file system block size */
	oskit_u32_t frsize;	/* fundamental file system block size */
	oskit_u32_t blocks;	/* total blocks in fs in units of frsize */
	oskit_u32_t bfree;	/* free blocks in fs */
	oskit_u32_t bavail;	/* free blocks avail to non-superuser */
	oskit_u32_t files;	/* total file nodes in file system */
	oskit_u32_t ffree;	/* free file nodes in fs */
	oskit_u32_t favail;	/* free file nodes avail to non-superuser */
	oskit_u32_t fsid;	/* file system id */
	oskit_u32_t flag;	/* mount flags */	
	oskit_u32_t namemax;	/* max bytes in a file name */
};
typedef struct oskit_statfs oskit_statfs_t; 

/*
 * Bit flag definitions for the oskit_statfs.flag field.
 * CAE only defines rdonly and nosuid.
 * However, noexec and nodev seem reasonable to include as well.
 */
#define	OSKIT_FS_RDONLY	0x00000001	/* read only filesystem */
#define	OSKIT_FS_NOEXEC	0x00000002	/* can't exec from filesystem */
#define	OSKIT_FS_NOSUID	0x00000004	/* don't honor setuid bits on fs */
#define	OSKIT_FS_NODEV	0x00000008	/* don't interpret special files */


struct oskit_dir;
struct oskit_file;

/*
 * Basic file system object interface,
 * IID 4aa7df93-7c74-11cf-b500-08000953adc2.
 */
struct oskit_filesystem {
	struct oskit_filesystem_ops *ops;
};
typedef struct oskit_filesystem oskit_filesystem_t;

struct oskit_filesystem_ops {

	/*** COM-specified IUnknown interface operations ***/
	OSKIT_COMDECL	(*query)(oskit_filesystem_t *f,
				 const struct oskit_guid *iid,
				 void **out_ihandle);
	OSKIT_COMDECL_U	(*addref)(oskit_filesystem_t *f);
	OSKIT_COMDECL_U	(*release)(oskit_filesystem_t *f);

	/*** File System interface operations ***/

	/*
	 * Return general information about the file system
	 * on which this node resides.
	 */
	OSKIT_COMDECL	(*statfs)(oskit_filesystem_t *f,
				  oskit_statfs_t *out_stats);

	/*
	 * Write all data back to permanent storage.
	 * If wait is true, this call should not return
	 * until all pending data have been completely written;
	 * if wait is false, begin the I/O but do not wait
	 * to verify that it completes successfully.
	 */
	OSKIT_COMDECL	(*sync)(oskit_filesystem_t *f, oskit_bool_t wait);

	/*
	 * Return a reference to the root directory of this file system.
	 */
	OSKIT_COMDECL	(*getroot)(oskit_filesystem_t *f,
				   struct oskit_dir **out_dir);

	/*
	 * Update the flags of this filesystem (eg rdonly -> rdwr). 
	 * May return OSKIT_E_NOTIMPL if this is not
	 * a typical disk-based file system.
	 */
	OSKIT_COMDECL	(*remount)(oskit_filesystem_t *f,
				   oskit_u32_t flags);

	/*
	 * Forcibly unmount this filesystem.  
	 *
	 * A normal unmount occurs when all references
	 * to the filesystem are released.  This
	 * operation forces the filesystem to be
	 * unmounted, even if references still exist,
	 * and is consequently unsafe.
	 *
	 * Subsequent attempts to use references to the
	 * filesystem or to use references to files within
	 * the filesystem may yield undefined results.
	 */
	OSKIT_COMDECL	(*unmount)(oskit_filesystem_t *f);

	/*
	 * Lookup an inode number and return an oskit_file_t *.
	 */
	OSKIT_COMDECL (*lookupi)(oskit_filesystem_t *f, oskit_ino_t ino,
				 struct oskit_file **out_file);
};

#define oskit_filesystem_query(f, iid, out_ihandle) \
	((f)->ops->query((oskit_filesystem_t *)(f), (iid), (out_ihandle)))
#define oskit_filesystem_addref(f) \
	((f)->ops->addref((oskit_filesystem_t *)(f)))
#define oskit_filesystem_release(f) \
	((f)->ops->release((oskit_filesystem_t *)(f)))
#define oskit_filesystem_statfs(f, out_stats) \
	((f)->ops->statfs((oskit_filesystem_t *)(f), (out_stats)))
#define oskit_filesystem_sync(f, wait) \
	((f)->ops->sync((oskit_filesystem_t *)(f), (wait)))
#define oskit_filesystem_getroot(f, out_dir) \
	((f)->ops->getroot((oskit_filesystem_t *)(f), (out_dir)))
#define oskit_filesystem_remount(f, flags) \
	((f)->ops->remount((oskit_filesystem_t *)(f), (flags)))
#define oskit_filesystem_unmount(f) \
	((f)->ops->unmount((oskit_filesystem_t *)(f)))
#define oskit_filesystem_lookupi(f, ino, out_file) \
	((f)->ops->lookupi((oskit_filesystem_t *)(f), (ino), (out_file)))

/* GUID for oskit_filesystem interface */
extern const struct oskit_guid oskit_filesystem_iid;
#define OSKIT_FILESYSTEM_IID OSKIT_GUID(0x4aa7df93, 0x7c74, 0x11cf, \
				0xb5, 0x00, 0x08, 0x00, 0x09, 0x53, 0xad, 0xc2)


#endif /* _OSKIT_COM_FILESYSTEM_H_ */
