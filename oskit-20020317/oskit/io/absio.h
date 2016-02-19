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
 * Absolute byte-oriented I/O interface definition.
 */
#ifndef _OSKIT_IO_ABSIO_H_
#define _OSKIT_IO_ABSIO_H_

#include <oskit/com.h>


/*
 * Simple byte-oriented absolute I/O interface,
 * IID 4aa7df9d-7c74-11cf-b500-08000953adc2.
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
 * This interface is identical to the blkio interface,
 * except that the "block size" is always assumed to be one.
 * The positions of the operations in the two interfaces are the same,
 * except the getblocksize() entrypoint in blkio
 * corresponds to a 'reserved' entry in absio;
 * this arrangement makes it trivial for an object to implement both interfaces
 * simply by providing a getblocksize() method that always returns one,
 * and returning the same pointer on queries for both blkio and absio.
 *
 * All offsets are 64-bit unsigned integers, to allow for large devices/files,
 * but read/write amounts are oskit_size_t (32 bits for 32-bit machines)
 * since more than that can't be handled at once using memory buffers anyway.
 *
 * XXX define semantics of error conditions and incomplete operations
 */
struct oskit_absio {
	struct oskit_absio_ops *ops;
};
typedef struct oskit_absio oskit_absio_t;

struct oskit_absio_ops {

	/*** COM-specified IUnknown interface operations ***/
	OSKIT_COMDECL	(*query)(oskit_absio_t *io,
				 const struct oskit_guid *iid,
				 void **out_ihandle);
	OSKIT_COMDECL_U	(*addref)(oskit_absio_t *io);
	OSKIT_COMDECL_U	(*release)(oskit_absio_t *io);

	/*** Operations specific to the absolute block I/O interface ***/

	/*
	 * Reserved for the getblocksize() method,
	 * for the convenience of objects that want to support
	 * both the blkio and the absio interfaces.
	 */
	void		*reserved;

	/*
	 * Read data from this I/O object starting at an absolute offset.
	 * If an attempt is made to read past the end of the object,
	 * the returned 'actual' will be less than the requested 'amount'.
	 */
	OSKIT_COMDECL	(*read)(oskit_absio_t *io, void *buf,
				oskit_off_t offset, oskit_size_t amount,
				oskit_size_t *out_actual);

	/*
	 * Write data to this I/O object starting at an absolute offset.
	 * If an attempt is made to write past the end of the object,
	 * and the object cannot be expanded
	 *(e.g., it's fixed-size or all available storage is used),
	 * the returned 'actual' will be less than the requested 'amount'.
	 */
	OSKIT_COMDECL	(*write)(oskit_absio_t *io, const void *buf,
				 oskit_off_t offset, oskit_size_t amount,
				 oskit_size_t *out_actual);

	/*
	 * Return the current size of the block I/O object in bytes.
	 * Absolute I/O objects are often fixed-size, but may be variable size,
	 * in which case this can return different values at different times.
	 */
	OSKIT_COMDECL	(*getsize)(oskit_absio_t *io, oskit_off_t *out_size);

	/*
	 * Set the size of the absolute I/O object to the specified value.
	 * If the new size is smaller than the existing size,
	 * the extra data at the end of the object is lost;
	 * if the new size is larger, the new data is read as zeros.
	 * Fails if the I/O object is fixed-size.
	 */
	OSKIT_COMDECL	(*setsize)(oskit_absio_t *io, oskit_off_t new_size);
};

/* GUID for absio interface */
extern const struct oskit_guid oskit_absio_iid;
#define OSKIT_ABSIO_IID OSKIT_GUID(0x4aa7df9d, 0x7c74, 0x11cf, \
		0xb5, 0x00, 0x08, 0x00, 0x09, 0x53, 0xad, 0xc2)

#define oskit_absio_query(io, iid, out_ihandle) \
	((io)->ops->query((oskit_absio_t *)(io), (iid), (out_ihandle)))
#define oskit_absio_addref(io) \
	((io)->ops->addref((oskit_absio_t *)(io)))
#define oskit_absio_release(io) \
	((io)->ops->release((oskit_absio_t *)(io)))
#define oskit_absio_read(io, buf, offset, amount, out_actual) \
	((io)->ops->read((oskit_absio_t *)(io), (buf), (offset), (amount), (out_actual)))
#define oskit_absio_write(io, buf, offset, amount, out_actual) \
	((io)->ops->write((oskit_absio_t *)(io), (buf), (offset), (amount), (out_actual)))
#define oskit_absio_getsize(io, out_size) \
	((io)->ops->getsize((oskit_absio_t *)(io), (out_size)))
#define oskit_absio_setsize(io, new_size) \
	((io)->ops->setsize((oskit_absio_t *)(io), (new_size)))

#endif /* _OSKIT_IO_ABSIO_H_ */
