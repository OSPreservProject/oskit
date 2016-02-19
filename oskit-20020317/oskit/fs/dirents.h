/*
 * Copyright (c) 1999,  University of Utah and the Flux Group.
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
 * Definition of the COM oskit_dirents interface, which represents 
 * a dirents buffer.
 */
#ifndef _OSKIT_FS_DIRENTS_H_
#define _OSKIT_FS_DIRENTS_H_

#include <oskit/com.h>

/*
 * A single directory entry. The caller is responsible for passing in
 * a pointer to the following structure, which must be big enough to
 * accomodate the name string, as indicated by the namelen field that
 * is set by the caller. The caller might determine the optimal size
 * of this buffer via the namemax field of the statfs operation, or
 * it could just guess and look for a OSKIT_ENAMETOOLONG error return
 * code, and then try again with a bigger size. The namelen field is
 * set to the actual length of the name on return from the call. The
 * ino field is supposed to represent an inode number.
 */
struct oskit_dirent {
	oskit_size_t	namelen;		/* Size of name buffer  */
	oskit_ino_t	ino;			/* entry inode */
	char		name[0];		/* entry name */
};
typedef struct oskit_dirent oskit_dirent_t;

/*
 * oskit_dirents_t: A COM object representing a group of oskit_dirent_t
 * structures. The caller reads out individual dirent entries (above
 * structure), one at a time, and then releases the object when it has them
 * all, or no longer cares (closedir). This keeps the memory allocation and
 * deallocation details internal to the filesystem implementation, avoiding
 * the "what deallocator should I use" confusion.
 *
 * IID 4aa7dffc-7c74-11cf-b500-08000953adc2.
 */
struct oskit_dirents {
	struct oskit_dirents_ops *ops;
};
typedef struct oskit_dirents oskit_dirents_t;

struct oskit_dirents_ops {
	/*** Operations inherited from oskit_unknown interface ***/
	OSKIT_COMDECL_IUNKNOWN(oskit_dirents_t)

	/*** Operations specific to the oskit_dirents interface ***/

	/*
	 * Return the count of dirent entries in the object.
	 */
	OSKIT_COMDECL	(*getcount)(oskit_dirents_t *d, int *out_count);

	/*
	 * Get the next directory entry. The caller passes in a properly
	 * sized and initialized oskit_dirent_t structure above (making
	 * sure to set the namelen field). Upon return, the string buffer
	 * contains a null terminated string, ino contains a filesystem
	 * specific value, and namelen contains the true length of the name.
	 * Returns OSKIT_ENAMETOOLONG if the buffer provided is not big
	 * enough to hold the filename. Returns OSKIT_EWOULDBLOCK is there
	 * are no more entries to read.
	 */
	OSKIT_COMDECL	(*getnext)(oskit_dirents_t *d, oskit_dirent_t *dirent);

	/*
	 * Rewind the getnext position back to the beginning so that the
	 * next getnext call starts with the first dirent in the object.
	 */
	OSKIT_COMDECL	(*rewind)(oskit_dirents_t *d);
};

#define oskit_dirents_query(d, iid, out_ihandle) \
	((d)->ops->query((oskit_dirents_t *)(d), (iid), (out_ihandle)))
#define oskit_dirents_addref(d) \
	((d)->ops->addref((oskit_dirents_t *)(d)))
#define oskit_dirents_release(d) \
	((d)->ops->release((oskit_dirents_t *)(d)))

#define oskit_dirents_getcount(d, out_count) \
	((d)->ops->getcount((oskit_dirents_t *)(d), (out_count)))
#define oskit_dirents_getnext(d, dirent) \
	((d)->ops->getnext((oskit_dirents_t *)(d), (dirent)))
#define oskit_dirents_rewind(d) \
	((d)->ops->rewind((oskit_dirents_t *)(d)))


/* GUID for oskit_dirents interface */
extern const struct oskit_guid oskit_dirents_iid;
#define OSKIT_DIRENTS_IID OSKIT_GUID(0x4aa7dffc, 0x7c74, 0x11cf, \
			0xb5, 0x00, 0x08, 0x00, 0x09, 0x53, 0xad, 0xc2)

#endif /* _OSKIT_FS_DIR_H_ */
