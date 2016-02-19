/*
 * Copyright (c) 1999 University of Utah and the Flux Group.
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
 * Definition of the COM filesystem namespace object. This object handles
 * namei translation from paths to oskit_dir_t/oskit_file_t. It also handles
 * mountpoints. It is intended to be interposed between the actual
 * filesystems and the POSIX library, thereby keeping all of the lookup code
 * out of the POSIX library. The current croot and cwd are contained within
 * the object to avoid having to pass them into the object on every lookup,
 * and also makes it easy to clone the namespace object for another process
 * so that it has its own root/cwd it can change.
 */
#ifndef _OSKIT_COM_FSNAMESPACE_H_
#define _OSKIT_COM_FSNAMESPACE_H_

#include <oskit/com.h>

struct oskit_dir;
struct oskit_file;

/* lookup flags */
#define FSLOOKUP_NOFLAGS	0x00	/* Normal operation */
#define FSLOOKUP_FOLLOW		0x00	/* Normal operation */
#define FSLOOKUP_NOFOLLOW	0x01	/* Do not follow last symlink */
#define FSLOOKUP_NOCACHE	0x02	/* Do not leave entry in the cache */
#define FSLOOKUP_PARENT		0x04	/* Return parent instead */

/*
 * Filesystem namespace object interface,
 * IID 4aa7dff8-7c74-11cf-b500-08000953adc2.
 */
struct oskit_fsnamespace {
	struct oskit_fsnamespace_ops *ops;
};
typedef struct oskit_fsnamespace oskit_fsnamespace_t;

struct oskit_fsnamespace_ops {

	/*** COM-specified IUnknown interface operations ***/
	OSKIT_COMDECL_IUNKNOWN(oskit_fsnamespace_t)

	/*** FileSystem Namespace interface operations ***/

	/*
	 * Change the root directory. Subsequent absolute path lookups
	 * will be rooted here. 
	 */
	OSKIT_COMDECL	(*chroot)(oskit_fsnamespace_t *f, const char *dirname);

	/*
	 * Change the current working directory (cwd). Subsequent relative
	 * path lookups will start here. 
	 */
	OSKIT_COMDECL	(*chcwd)(oskit_fsnamespace_t *f,
				struct oskit_dir *cwd);

	/*
	 * Lookup a name in the filesystem. Standard name translation,
	 * subject to flags described below. 
	 */
	OSKIT_COMDECL	(*lookup)(oskit_fsnamespace_t *f,
				const char *filename, int flags,
				struct oskit_file **out_file);
		
	/*
	 * Create a mountpoint in the filesystem. A directory subtree is
	 * mounted over the directory named by mountdir. A lookup operation
	 * is performed on mountdir to find the mounted over oskir_dir_t.
	 */
	OSKIT_COMDECL	(*mount)(oskit_fsnamespace_t *f,
				const char *mountdir,
				struct oskit_file *subtree);

	/*
	 * Undo a mountpoint in the filesystem. 
	 */
	OSKIT_COMDECL	(*unmount)(oskit_fsnamespace_t *f,
				const char *mountpoint);

	/*
	 * Clone an fsnamespace object. Typically used to give a child
	 * its own view of the filesystem namespace (root/cwd). All else
	 * stays the same of course.
	 */
	OSKIT_COMDECL	(*clone)(oskit_fsnamespace_t *f,
				oskit_fsnamespace_t **out_fsnamespace);

};

#define oskit_fsnamespace_query(f, iid, out_ihandle) \
	((f)->ops->query((oskit_fsnamespace_t *)(f), (iid), (out_ihandle)))
#define oskit_fsnamespace_addref(f) \
	((f)->ops->addref((oskit_fsnamespace_t *)(f)))
#define oskit_fsnamespace_release(f) \
	((f)->ops->release((oskit_fsnamespace_t *)(f)))

#define oskit_fsnamespace_chroot(f, dirname) \
	((f)->ops->chroot((oskit_fsnamespace_t *)(f), (dirname)))
#define oskit_fsnamespace_chcwd(f, cwd) \
	((f)->ops->chcwd((oskit_fsnamespace_t *)(f), (cwd)))
#define oskit_fsnamespace_lookup(f, flags, int, out_file) \
	((f)->ops->lookup((oskit_fsnamespace_t *)(f), \
				(flags), (int), (out_file)))
#define oskit_fsnamespace_mount(f, mountdir, subtree) \
	((f)->ops->mount((oskit_fsnamespace_t *)(f), (mountdir), (subtree)))
#define oskit_fsnamespace_unmount(f, mountpoint) \
	((f)->ops->unmount((oskit_fsnamespace_t *)(f), (mountpoint)))
#define oskit_fsnamespace_clone(f, out_f) \
	((f)->ops->clone((oskit_fsnamespace_t *)(f), (out_f)))

/* GUID for oskit_fsnamespace interface */
extern const struct oskit_guid oskit_fsnamespace_iid;
#define OSKIT_FSNAMESPACE_IID OSKIT_GUID(0x4aa7dff8, 0x7c74, 0x11cf, \
				0xb5, 0x00, 0x08, 0x00, 0x09, 0x53, 0xad, 0xc2)

/*
 * Create a new fsnamespace.
 */
oskit_error_t	oskit_create_fsnamespace(struct oskit_dir *rootdir,
				struct oskit_dir *cwd,
				oskit_fsnamespace_t **out_fsnamespace);

#endif /* _OSKIT_COM_FSNAMESPACE_H_ */
