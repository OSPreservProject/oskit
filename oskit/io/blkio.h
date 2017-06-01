/*
 * Copyright (c) 1996-1998 University of Utah and the Flux Group.
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
 * Block I/O interface definition.
 */
#ifndef _OSKIT_IO_BLKIO_H_
#define _OSKIT_IO_BLKIO_H_

#include <oskit/com.h>


/*
 * Simple absolute block I/O interface,
 * IID 4aa7df81-7c74-11cf-b500-08000953adc2.
 *
 * I/O buffers in this interface are handled basically POSIX-style,
 * with the caller supplying a buffer locally accessible to the object.
 * However, this interface does _not_ have a notion of a "current position",
 * so it is potentially applicable to "per-storage-container" objects
 * and not just "per-open" objects (though it can be presented by either).
 * Thus, buffer management is simple and well-understood,
 * though not optimal in many cases.
 * Objects can export additional interfaces for better performance.
 * 
 * Objects exporting this interface may have a minimum block size,
 * which is always a power of two;
 * clients using this interface must only attempt to read and write data
 * in units of this block size.
 * If the object does not have a minimum block size,
 * then getblocksize() will return a block size of one; in that case,
 * the object may want to export the absio interface as well (see absio.h).
 * In any case, the block size is expected to remain constant
 * for the lifetime of the block I/O object.
 *
 * All offsets are 64-bit unsigned integers, to allow for large devices/files,
 * but read/write amounts are oskit_size_t (32 bits for 32-bit machines)
 * since more than that can't be handled at once using memory buffers anyway.
 *
 * XXX define semantics of error conditions and incomplete operations
 */
struct oskit_blkio {
	struct oskit_blkio_ops *ops;
};
typedef struct oskit_blkio oskit_blkio_t;

struct oskit_blkio_ops {

	/*** COM-specified IUnknown interface operations ***/
	OSKIT_COMDECL	(*query)(oskit_blkio_t *io,
				 const struct oskit_guid *iid,
				 void **out_ihandle);
	OSKIT_COMDECL_U	(*addref)(oskit_blkio_t *io);
	OSKIT_COMDECL_U	(*release)(oskit_blkio_t *io);

	/*** Operations specific to the absolute block I/O interface ***/

	/*
	 * Return the minimum block size of this block I/O object,
	 * which a power of two and constant for the lifetime of the object.
	 * All reads and writes must use multiples of this block size.
	 */
	OSKIT_COMDECL_U	(*getblocksize)(oskit_blkio_t *io);

	/*
	 * Read data from this I/O object starting at an absolute offset.
	 * If an attempt is made to read past the end of the object,
	 * the returned 'actual' will be less than the requested 'amount'.
	 */
	OSKIT_COMDECL	(*read)(oskit_blkio_t *io, void *buf,
				oskit_off_t offset, oskit_size_t amount,
				oskit_size_t *out_actual);

	/*
	 * Write data to this I/O object starting at an absolute offset.
	 * If an attempt is made to write past the end of the object,
	 * and the object cannot be expanded
	 *(e.g., it's fixed-size or all available storage is used),
	 * the returned 'actual' will be less than the requested 'amount'.
	 */
	OSKIT_COMDECL	(*write)(oskit_blkio_t *io, const void *buf,
				 oskit_off_t offset, oskit_size_t amount,
				 oskit_size_t *out_actual);

	/*
	 * Return the current size of the block I/O object in bytes.
	 * Block I/O objects are often fixed-size, but may be variable size,
	 * in which case this can return different values at different times.
	 */
	OSKIT_COMDECL	(*getsize)(oskit_blkio_t *io, oskit_off_t *out_size);

	/*
	 * Set the size of the block I/O object to the specified value.
	 * If the new size is smaller than the existing size,
	 * the extra data at the end of the object is lost;
	 * if the new size is larger, the new data is read as zeros.
	 * Fails if the block I/O object is fixed-size.
	 */
	OSKIT_COMDECL	(*setsize)(oskit_blkio_t *io, oskit_off_t new_size);
};

/* GUID for oskit_blkio interface */
extern const struct oskit_guid oskit_blkio_iid;
#define OSKIT_BLKIO_IID OSKIT_GUID(0x4aa7df81, 0x7c74, 0x11cf, \
		0xb5, 0x00, 0x08, 0x00, 0x09, 0x53, 0xad, 0xc2)

#define oskit_blkio_query(io, iid, out_ihandle) \
	((io)->ops->query((oskit_blkio_t *)(io), (iid), (out_ihandle)))
#define oskit_blkio_addref(io) \
	((io)->ops->addref((oskit_blkio_t *)(io)))
#define oskit_blkio_release(io) \
	((io)->ops->release((oskit_blkio_t *)(io)))
#define oskit_blkio_getblocksize(io) \
	((io)->ops->getblocksize((oskit_blkio_t *)(io)))
#define oskit_blkio_read(io, buf, offset, amount, out_actual) \
	((io)->ops->read((oskit_blkio_t *)(io), (buf), (offset), (amount), (out_actual)))
#define oskit_blkio_write(io, buf, offset, amount, out_actual) \
	((io)->ops->write((oskit_blkio_t *)(io), (buf), (offset), (amount), (out_actual)))
#define oskit_blkio_getsize(io, out_size) \
	((io)->ops->getsize((oskit_blkio_t *)(io), (out_size)))
#define oskit_blkio_setsize(io, new_size) \
	((io)->ops->setsize((oskit_blkio_t *)(io), (new_size)))

/*
 * Create a blkio object representing a subset of another blkio object.
 * The resulting blkio object has fixed size (setsize doesn't work),
 * and has the same minimum block size as the underlying I/O object.
 * The specified offset and size must be multiples of this block size.
 * This facility is provided as part of the OSKIT's COM support library.
 */
oskit_error_t oskit_blkio_subset(oskit_blkio_t *io, oskit_off_t offset,
				oskit_off_t size, oskit_blkio_t **out_io);

#endif /* _OSKIT_IO_BLKIO_H_ */
