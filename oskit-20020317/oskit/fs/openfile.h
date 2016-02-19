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
 * Definition of the COM oskit_openfile interface,
 * which represents a file handle on a regular file
 * or another stream-like file (e.g., FIFO, pipe, socket).
 */
#ifndef _OSKIT_COM_OPENFILE_H_
#define _OSKIT_COM_OPENFILE_H_

#include <oskit/com/stream.h>
#include <oskit/fs/file.h>

/*
 * Basic open-file object interface,
 * IID 4aa7df9a-7c74-11cf-b500-08000953adc2.
 * This interface extends the stream interface.
 *
 * In addition to stream operations, an open-file
 * may support absolute IO operations; this
 * can be determined by querying for oskit_absio_iid.
 */
struct oskit_openfile {
	struct oskit_openfile_ops *ops;
};
typedef struct oskit_openfile oskit_openfile_t;

struct oskit_openfile_ops {

	/*** COM-specified IUnknown interface operations ***/
	OSKIT_COMDECL	(*query)(oskit_openfile_t *f,
				 const struct oskit_guid *iid,
				 void **out_ihandle);
	OSKIT_COMDECL_U	(*addref)(oskit_openfile_t *f);
	OSKIT_COMDECL_U	(*release)(oskit_openfile_t *f);


	/*** Operations inherited from oskit_stream interface ***/
	OSKIT_COMDECL	(*read)(oskit_openfile_t *f, void *buf, oskit_u32_t len,
				oskit_u32_t *out_actual);
	OSKIT_COMDECL	(*write)(oskit_openfile_t *f, const void *buf,
				 oskit_u32_t len, oskit_u32_t *out_actual);
	OSKIT_COMDECL	(*seek)(oskit_openfile_t *f, oskit_s64_t ofs,
				oskit_seek_t whence, oskit_u64_t *out_newpos);
	OSKIT_COMDECL	(*setsize)(oskit_openfile_t *f, oskit_u64_t new_size);
	OSKIT_COMDECL	(*copyto)(oskit_openfile_t *f, oskit_stream_t *dst,
				   oskit_u64_t size,
				   oskit_u64_t *out_read,
				   oskit_u64_t *out_written);
	OSKIT_COMDECL	(*commit)(oskit_openfile_t *f, oskit_u32_t commit_flags);
	OSKIT_COMDECL	(*revert)(oskit_openfile_t *f);
	OSKIT_COMDECL	(*lockregion)(oskit_openfile_t *f,
					oskit_u64_t offset, oskit_u64_t size,
					oskit_u32_t lock_type);
	OSKIT_COMDECL	(*unlockregion)(oskit_openfile_t *f,
					 oskit_u64_t offset, oskit_u64_t size,
					 oskit_u32_t lock_type);
	OSKIT_COMDECL	(*stat)(oskit_openfile_t *f, oskit_stream_stat_t *out_stat,
				oskit_u32_t stat_flags);
	OSKIT_COMDECL	(*clone)(oskit_openfile_t *f, oskit_openfile_t **out_stream);

	/*** Operations specific to oskit_openfile ***/

	/*
	 * Obtain the VFS-style file object to which this per-open
	 * file object refers.
	 */ 
	OSKIT_COMDECL	(*getfile)(oskit_openfile_t *f, 
				   oskit_file_t **out_file);	
};

#define oskit_openfile_query(f, iid, out_ihandle) \
	((f)->ops->query((oskit_openfile_t *)(f), (iid), (out_ihandle)))
#define oskit_openfile_addref(f) \
	((f)->ops->addref((oskit_openfile_t *)(f)))
#define oskit_openfile_release(f) \
	((f)->ops->release((oskit_openfile_t *)(f)))
#define oskit_openfile_read(f, buf, len, out_actual) \
	((f)->ops->read((oskit_openfile_t *)(f), (buf), (len), (out_actual)))
#define oskit_openfile_write(f, buf, len, out_actual) \
	((f)->ops->write((oskit_openfile_t *)(f), (buf), (len), (out_actual)))
#define oskit_openfile_seek(f, ofs, whence, out_newpos) \
	((f)->ops->seek((oskit_openfile_t *)(f), (ofs), (whence), (out_newpos)))
#define oskit_openfile_setsize(f, new_size) \
	((f)->ops->setsize((oskit_openfile_t *)(f), (new_size)))
#define oskit_openfile_copyto(f, dst, size, out_read, out_written) \
	((f)->ops->copyto((oskit_openfile_t *)(f), (dst), (size), (out_read), (out_written)))
#define oskit_openfile_commit(f, commit_flags) \
	((f)->ops->commit((oskit_openfile_t *)(f), (commit_flags)))
#define oskit_openfile_revert(f) \
	((f)->ops->revert((oskit_openfile_t *)(f)))
#define oskit_openfile_lockregion(f, offset, size, lock_type) \
	((f)->ops->lockregion((oskit_openfile_t *)(f), (offset), (size), (lock_type)))
#define oskit_openfile_unlockregion(f, offset, size, lock_type) \
	((f)->ops->unlockregion((oskit_openfile_t *)(f), (offset), (size), (lock_type)))
#define oskit_openfile_stat(f, out_stat, stat_flags) \
	((f)->ops->stat((oskit_openfile_t *)(f), (out_stat), (stat_flags)))
#define oskit_openfile_clone(f, out_stream) \
	((f)->ops->clone((oskit_openfile_t *)(f), (out_stream)))
#define oskit_openfile_getfile(f, out_file) \
	((f)->ops->getfile((oskit_openfile_t *)(f), (out_file)))

/* GUID for oskit_openfile interface */
extern const struct oskit_guid oskit_openfile_iid;
#define OSKIT_OPENFILE_IID OSKIT_GUID(0x4aa7df9a, 0x7c74, 0x11cf, \
				0xb5, 0x00, 0x08, 0x00, 0x09, 0x53, 0xad, 0xc2)


/*** Utility objects related to the oskit_openfile interface ***/

struct blkio;

/*
 * Create an open-file object representing a blkio data stream.
 * The original blkio interface is also exposed by the new object.
 * The statistics information to be returned by the stat() method
 * can be provided as a parameter to this creation function;
 * this stat structure will be returned unmodified from stat calls
 * except for the st_size element, which is obtained from the blkio object.
 * If the stat parameter is NULL, then reasonable defaults are provided.
 */
oskit_error_t openoskit_file_from_blkio(struct blkio *bio, 
					oskit_oflags_t flags,
					const oskit_stat_t *stat,
					oskit_openfile_t **out_file);
oskit_error_t adapt_oskit_file_from_blkio(struct blkio *bio,
					oskit_openfile_t **out_file);


#endif /* _OSKIT_COM_OPENFILE_H_ */
