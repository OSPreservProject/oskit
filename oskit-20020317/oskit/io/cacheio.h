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
 * Cached block I/O interface definitions.
 * XXX Note: This interface definition isn't complete or even functional yet.
 */
#ifndef _OSKIT_IO_CACHEIO_H_
#define _OSKIT_IO_CACHEIO_H_

#include <oskit/types.h>
#include <oskit/com.h>

/*
 * Cached block I/O interface,
 * IID 4aa7df9e-7c74-11cf-b500-08000953adc2.
 */
struct oskit_cacheio {
	struct oskit_cacheio_ops *ops;
};
typedef struct oskit_cacheio oskit_cacheio_t;

struct oskit_cacheio_ops {

	/* Methods inherited from IUnknown interface */
	OSKIT_COMDECL	(*query)(oskit_cacheio_t *io,
				 const struct oskit_guid *iid,
				 void **out_ihandle);
	OSKIT_COMDECL_U	(*addref)(oskit_cacheio_t *io);
	OSKIT_COMDECL_U	(*release)(oskit_cacheio_t *io);

	/*
	 * Return the minimum block size of this cached I/O object,
	 * which a power of two and constant for the lifetime of the object.
	 * The offsets and sizes of all cache buffers
	 * are multiples of this block size.
	 */
	OSKIT_COMDECL	(*getblocksize)(oskit_cacheio_t *io);

/*	find_block

	get_cache_size
	set_cache_size */
};

extern const struct oskit_guid oskit_cacheio_iid;
#define OSKIT_CACHEIO_IID OSKIT_GUID(0x4aa7df9f, 0x7c74, 0x11cf, \
                0xb5, 0x00, 0x08, 0x00, 0x09, 0x53, 0xad, 0xc2)


/*
 * Cache buffer interface,
 * IID 4aa7df8f-7c74-11cf-b500-08000953adc2.
 * This interface is a subtype of the absio interface;
 * objects exporting bufio automatically export absio as well.
 * This interface is intended to be exported by objects
 * that generally represent an in-memory buffer of some kind,
 * whose contents can often be accessed directly
 * by the processor or by device hardware.
 *
 * Only the read/write methods, inherited from absio, are mandatory;
 * the others may consistently fail with OSKIT_E_NOTIMPL
 * if they cannot be implemented efficiently in a particular situation.
 * In that case, the caller must use read/write instead to copy the data.
 * In general, the map and wire methods should only be implemented
 * if they can be done more efficiently than simply copying the data.
 * Further, even if a buffer I/O implementation does implement map and/or wire,
 * it may allow only one mapping or wiring to be in effect at once,
 * failing if the client attempts to map or wire the buffer a second time.
 *
 * A particular buffer may be semantically read-only or write-only;
 * it is assumed that parties passing bufio objects around
 * will agree upon this as part of their protocols.
 * For a read-only buffer, the write() method may or may not fail,
 * and a mapping established using the map() method may or may not be read-only;
 * it is the client's responsibility not to attempt to write to the buffer.
 * Similarly, for a write-only buffer, the read() method may or may not fail;
 * it is the client's responsibility not to attempt to read from the buffer.
 */
struct oskit_bufio {
	struct oskit_bufio_ops *ops;
};
typedef struct oskit_bufio oskit_bufio_t;

struct oskit_bufio_ops {
	/* COM-specified IUnknown interface operations */
	OSKIT_COMDECL	(*query)(oskit_bufio_t *dev,
				 const struct oskit_guid *iid,
				 void **out_ihandle);
	OSKIT_COMDECL_U	(*addref)(oskit_bufio_t *dev);
	OSKIT_COMDECL_U	(*release)(oskit_bufio_t *dev);

	/*** Methods inherited from absio ***/

	/*
	 * Reserved for the getblocksize() method,
	 * for the convenience of objects that want to support
	 * both the blkio and the absio/bufio interfaces.
	 */
	void		*reserved;

	/*
	 * Read data from this I/O object starting at an absolute offset.
	 * If an attempt is made to read past the end of the object,
	 * the returned 'actual' will be less than the requested 'amount',
	 * but no error is returned unless no data could be read.
	 */
	OSKIT_COMDECL	(*read)(oskit_bufio_t *io, void *buf,
				oskit_off_t offset, oskit_size_t amount,
				oskit_size_t *out_actual);

	/*
	 * Write data to this I/O object starting at an absolute offset.
	 * If an attempt is made to write past the end of the object,
	 * and the object cannot be expanded
	 * (e.g., it's fixed-size or all available storage is used),
	 * the returned 'actual' will be less than the requested 'amount',
	 * but no error is returned unless no data could be written.
	 */
	OSKIT_COMDECL	(*write)(oskit_bufio_t *io, const void *buf,
				 oskit_off_t offset, oskit_size_t amount,
				 oskit_size_t *out_actual);

	/*
	 * Return the current size of the buffer object, in bytes.
	 * Buffer I/O objects are often fixed-size, but may be variable size,
	 * in which case this can return different values at different times.
	 */
	OSKIT_COMDECL	(*getsize)(oskit_bufio_t *io, oskit_off_t *out_size);

	/*
	 * Set the size of the buffer to the specified value.
	 * If the new size is smaller than the existing size,
	 * the extra data at the end of the buffer is lost;
	 * if the new size is larger, the new data is read as zeros.
	 * Fails if the buffer object is fixed-size.
	 */
	OSKIT_COMDECL	(*setsize)(oskit_bufio_t *io, oskit_off_t new_size);


	/*** Methods specific to bufio ***/

	/*
	 * Map some or all of this buffer into locally accessible memory,
	 * so that the client can access it using pointer arithmetic.
	 * The mapping is not guaranteed to have any particular alignment.
	 * This method does not necessarily twiddle with virtual memory;
	 * it may just return a pointer to a buffer
	 * if the data is already in locally-accessible memory.
	 */
	OSKIT_COMDECL	(*map)(oskit_bufio_t *io, void **out_addr,
			       oskit_addr_t offset, oskit_size_t count);

	/*
	 * Unmap a region of this buffer previously mapped with map().
	 * The parameters must be _exactly_ as used in the map() call.
	 */
	OSKIT_COMDECL	(*unmap)(oskit_bufio_t *io, void *addr,
				 oskit_addr_t offset, oskit_size_t count);

	/*
	 * Wire down a region of this buffer into contiguous physical memory,
	 * so that it can be accessed through hardware DMA.
	 * The physical address of the buffer
	 * is guaranteed not to change or otherwise become invalid
	 * until the region is unwired or the bufio object is released.
	 * The wired buffer is not guaranteed
	 * to have any particular alignment or location properties:
	 * for example, if the device needs 16MB memory on a PC,
	 * then it must be prepared to use appropriate bounce buffers
	 * if the wired buffer turns out to be above 16MB.
	 */
	OSKIT_COMDECL	(*wire)(oskit_bufio_t *io, oskit_addr_t *out_phys_addr,
				oskit_addr_t offset, oskit_size_t count);

	/*
	 * Unwire a region of this buffer previously wired with wire().
	 * The parameters must be _exactly_ as used in the wire() call.
	 */
	OSKIT_COMDECL	(*unwire)(oskit_bufio_t *io, oskit_addr_t phys_addr,
				  oskit_addr_t offset, oskit_size_t count);

	/*** Operations specific to the cacheio_buf interface ***/

/*	prefetch
	fetch
	lock
	dirty
	unlock
	get_status

	(see ~/mdoc/mom/mom/NOTES) */
};


extern const struct oskit_guid oskit_bufio_iid;
#define OSKIT_BUFIO_IID OSKIT_GUID(0x4aa7df8d, 0x7c74, 0x11cf, \
                0xb5, 0x00, 0x08, 0x00, 0x09, 0x53, 0xad, 0xc2)


/*
 * This function provides a default cacheio object implementation,
 * in which the buffer is simply a block of ordinary heap memory.
 * The buffer's initial contents is undefined.
 * This implementation is only a convenience; its use is not mandatory.
 * This implementation supports the map() method
 * (it simply returns a direct pointer to the allocated buffer),
 * but the wire() method always returns E_NOTIMPL.
 * Returns NULL if there is not enough memory to create the bufio object.
 */
oskit_bufio_t *oskit_cacheio_create(oskit_size_t size);

#endif /* _OSKIT_IO_CACHEIO_H_ */
