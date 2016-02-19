/*
 * Copyright (c) 1997-2001 University of Utah and the Flux Group.
 * All rights reserved.
 * 
 * This file is part of the OSKit Linux Glue Libraries, which are free
 * software, also known as "open source;" you can redistribute them and/or
 * modify them under the terms of the GNU General Public License (GPL),
 * version 2, as published by the Free Software Foundation (FSF).
 * 
 * The OSKit is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GPL for more details.  You should have
 * received a copy of the GPL along with the OSKit; see the file COPYING.  If
 * not, write to the FSF, 59 Temple Place #330, Boston, MA 02111-1307, USA.
 */
/*
 * Definition of an extended buffer-oriented I/O interface,
 * which extends the basic bufio interface with one that is more
 * like a linux SKB. Why? Cause the generic bufio is nice and generic,
 * but gives poor performance in most applications. The specific differences
 * are that the underlying linux sk_buff structure can be mapped in and
 * operated on directly, instead of having to go through COM interfaces
 * to do trivial things (skb_put, skb_reserve, etc). A new "clone" method
 * is also supported that duplicates an SKB. A "copy" method might also
 * be added at some point.
 *
 * Implementors of an oskit_skbufio_t are expected to implement the
 * oskit_bufio_t interface as well.
 *
 * This file is highly dependent on linux/src/include/linux/sbuff.h,
 * especially with regards to the sk_buff structure defined below and
 * the assorted inlines based on it. 
 */
#ifndef _OSKIT_IO_SKBUFIO_H_
#define _OSKIT_IO_SKBUFIO_H_

#include <oskit/types.h>
#include <oskit/com.h>
#include <oskit/io/bufio.h>
struct sk_buff;

/*
 * SKB buffer I/O interface,
 * IID 4aa7df8d-7c74-11cf-b500-08000953adc2.
 */
struct oskit_skbufio {
	struct oskit_skbufio_ops *ops;
};
typedef struct oskit_skbufio oskit_skbufio_t;

struct oskit_skbufio_ops {
	/* COM-specified IUnknown interface operations */
	OSKIT_COMDECL_IUNKNOWN(oskit_skbufio_t)

	/*** Methods inherited from bufio ***/

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
	OSKIT_COMDECL	(*read)(oskit_skbufio_t *io, void *buf,
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
	OSKIT_COMDECL	(*write)(oskit_skbufio_t *io, const void *buf,
				 oskit_off_t offset, oskit_size_t amount,
				 oskit_size_t *out_actual);

	/*
	 * Return the current size of the buffer object, in bytes.
	 * Buffer I/O objects are often fixed-size, but may be variable size,
	 * in which case this can return different values at different times.
	 */
	OSKIT_COMDECL	(*getsize)(oskit_skbufio_t *io, oskit_off_t *out_size);

	/*
	 * Set the size of the buffer to the specified value.
	 * If the new size is smaller than the existing size,
	 * the extra data at the end of the buffer is lost;
	 * if the new size is larger, the new data is read as zeros.
	 * Fails if the buffer object is fixed-size.
	 */
	OSKIT_COMDECL	(*setsize)(oskit_skbufio_t *io, oskit_off_t new_size);


	/*** Methods specific to bufio ***/

	/*
	 * Map some or all of this buffer into locally accessible memory,
	 * so that the client can access it using pointer arithmetic.
	 * The mapping is not guaranteed to have any particular alignment.
	 * This method does not necessarily twiddle with virtual memory;
	 * it may just return a pointer to a buffer
	 * if the data is already in locally-accessible memory.
	 */
	OSKIT_COMDECL	(*map)(oskit_skbufio_t *io, void **out_addr,
			       oskit_off_t offset, oskit_size_t count);

	/*
	 * Unmap a region of this buffer previously mapped with map().
	 * The parameters must be _exactly_ as used in the map() call.
	 */
	OSKIT_COMDECL	(*unmap)(oskit_skbufio_t *io, void *addr,
				 oskit_off_t offset, oskit_size_t count);

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
	OSKIT_COMDECL	(*wire)(oskit_skbufio_t *io,
				oskit_addr_t *out_phys_addr,
				oskit_off_t offset, oskit_size_t count);

	/*
	 * Unwire a region of this buffer previously wired with wire().
	 * The parameters must be _exactly_ as used in the wire() call.
	 */
	OSKIT_COMDECL	(*unwire)(oskit_skbufio_t *io, oskit_addr_t phys_addr,
				  oskit_off_t offset, oskit_size_t count);

	/*
	 * Create a copy of the specified portion of this buffer,
	 * and return a reference to a new bufio object.
	 * This operation may perform optimizations such as copy-on-write.
	 */
	OSKIT_COMDECL	(*copy)(oskit_skbufio_t *io,
				oskit_off_t offset, oskit_size_t count,
				oskit_skbufio_t **out_io);

	/*
	 * Methods specific to oskit_skbufio_t
	 */
	
	/*
	 * metadata_map: Map in the sk_buff header structure. This is the
	 * linux resident sk_buff, which can then be operated on directly,
	 * instead of through the COM interfaces. That is, you get back
	 * a pointer to the sk_buff structure that is defined below. You
	 * are free to munge it as you like, but you do so at your own
	 * risk of course. NB: Do not muck with the fdev_count field, except
	 * when interrupts are disabled. Better to go through the normal
	 * addref/release interface.
	 *
	 * The version number argument is provided to guard against changes
	 * to the sk_buff structure below that would cause programs to break.
	 * Users of the metadata_map() method should pass in the constant
	 * SKB_VERSION_NUMBER (defined below) and check the return value.
	 */
	OSKIT_COMDECL	(*metadata_map)(oskit_skbufio_t *io,
				oskit_u32_t version, struct sk_buff **out_skb);

	/*
	 * metadata_unmap: Unmap the above. 
	 */
	OSKIT_COMDECL	(*metadata_unmap)(oskit_skbufio_t *io,
				struct sk_buff *addr);

	/*
	 * Clone: Clone an oskit_skbufio_t, return a handle on a new one.
	 */
	OSKIT_COMDECL	(*clone)(oskit_skbufio_t *io,
				oskit_skbufio_t **out_io);

	/*
	 * Cloned: Report whether the oskit_skbufio_t is a clone. Returns
	 * 1 in *cloned if it is.
	 */
	OSKIT_COMDECL	(*cloned)(oskit_skbufio_t *io, int *cloned);
};


extern const struct oskit_guid oskit_skbufio_iid;
#define OSKIT_SKBUFIO_IID OSKIT_GUID(0x4aa7dffe, 0x7c74, 0x11cf, \
                0xb5, 0x00, 0x08, 0x00, 0x09, 0x53, 0xad, 0xc2)

#define oskit_skbufio_query(dev, iid, out_ihandle) \
	((dev)->ops->query((oskit_skbufio_t *)(dev), (iid), (out_ihandle)))
#define oskit_skbufio_addref(dev) \
	((dev)->ops->addref((oskit_skbufio_t *)(dev)))
#define oskit_skbufio_release(dev) \
	((dev)->ops->release((oskit_skbufio_t *)(dev)))
#define oskit_skbufio_read(io, buf, offset, amount, out_actual) \
	((io)->ops->read((oskit_skbufio_t *)(io), (buf), (offset), \
			 (amount), (out_actual)))
#define oskit_skbufio_write(io, buf, offset, amount, out_actual) \
	((io)->ops->write((oskit_skbufio_t *)(io), (buf), (offset), \
			  (amount), (out_actual)))
#define oskit_skbufio_getsize(io, out_size) \
	((io)->ops->getsize((oskit_skbufio_t *)(io), (out_size)))
#define oskit_skbufio_setsize(io, new_size) \
	((io)->ops->setsize((oskit_skbufio_t *)(io), (new_size)))
#define oskit_skbufio_map(io, out_addr, offset, count) \
	((io)->ops->map((oskit_skbufio_t *)(io), (out_addr), (offset),(count)))
#define oskit_skbufio_unmap(io, addr, offset, count) \
	((io)->ops->unmap((oskit_skbufio_t *)(io), (addr), (offset), (count)))
#define oskit_skbufio_wire(io, out_phys_addr, offset, count) \
	((io)->ops->wire((oskit_skbufio_t *)(io), (out_phys_addr), \
			 (offset), (count)))
#define oskit_skbufio_unwire(io, phys_addr, offset, count) \
	((io)->ops->unwire((oskit_skbufio_t *)(io), (phys_addr), (offset), \
			   (count)))
#define oskit_skbufio_copy(io, offset, count, out_io) \
	((io)->ops->copy((oskit_skbufio_t *)(io), (offset), (count), (out_io)))


#define oskit_skbufio_metadata_map(io, vers, out_addr) \
	((io)->ops->metadata_map((oskit_skbufio_t *)(io), (vers), (out_addr)))
#define oskit_skbufio_metadata_unmap(io, addr) \
	((io)->ops->metadata_unmap((oskit_skbufio_t *)(io), (addr)))

#define oskit_skbufio_clone(io, out_io) \
	((io)->ops->clone((oskit_skbufio_t *)(io), (out_io)))
#define oskit_skbufio_cloned(io, iscloned) \
	((io)->ops->cloned((oskit_skbufio_t *)(io), (iscloned)))

/*
 * This function provides a default sbbufio buffer object implementation,
 * It makes sense that the linux code should provide this.
 *
 * 'Size' is the desired data size, 'hroom' the headroom, and 'troom' the
 * tailroom.  The total buffer size is the sum of the three.
 */
oskit_error_t
oskit_skbufio_create(oskit_size_t size, oskit_size_t hroom, oskit_size_t troom,
		     oskit_skbufio_t **out_skbufio);

/*
 * Allocate a buffer for use as an skbuff.
 * Returns a pointer to a buffer of at least the indicated size.  Also returns
 * the actual size of the buffer allocated.  Any additional space returned may
 * be used by the caller.  The caller should present the same size when freeing
 * the buffer.  Flags are OSKIT_MEM_ (aka OSENV_) allocation flags used to
 * indicate special memory needs.
 *
 * This routine may be called at hardware interrupt time and should protect
 * itself as necessary.
 */
void *oskit_skbufio_mem_alloc(oskit_size_t size, int mflags,
			      oskit_size_t *out_size);

/*
 * Free a buffer used as an skbuff.
 * The pointer and size passed should match those returned by a call to
 * oskit_skbufio_mem_alloc.
 *
 * This routine may be called at hardware interrupt time and should protect
 * itself as necessary.
 */
void oskit_skbufio_mem_free(void *ptr, oskit_size_t size);

/*
 * Define the OSKit version of the skbuf structure, The linux headers will
 * bring in this file in to define that structure. This provides a "clean"
 * header definition of the sk_buff structure that anyone can safely include.
 * An oskit_skbufio_t can be converted to its underlying sk_buff structure
 * (using the metadata map method), which can be operated on directly. 
 */

/*
 * We use this to enforce binary compatibility in metadata_map. If the
 * sk_buff structure changes, make sure this number is bumped so that
 * any program that calls metadata_map will fail gracefully instead of
 * silently.
 */
#define SKB_VERSION_NUMBER	2000070601

/*
 * SKB header size.  Used for memory allocation.
 */
#define SKB_ALIGN	7
#define SKB_HDRSIZE	((sizeof(struct sk_buff) + SKB_ALIGN) & ~SKB_ALIGN)

/*
 * These will not be used by code outside the linux module, so their
 * definitions are not necessary.
 */
struct sk_buff_head;
struct device;

#ifndef __KERNEL__
#include <oskit/machine/atomic.h>
#endif

struct sk_buff {
	struct sk_buff		*next;		/* Next buffer in list */
	struct sk_buff		*prev;		/* Previous buffer in list */
	struct sk_buff_head	*list;		/* List we are on */
	struct device		*dev;		/* Device ... */
	unsigned long		len;		/* Length of actual data */
	unsigned short		protocol;	/* Packet protocol */
	unsigned int		truesize;	/* Buffer size */
	unsigned char		*head;		/* Head of buffer */
	unsigned char		*data;		/* Data head pointer */
	unsigned char		*tail;		/* Tail pointer */
	unsigned char		*end;		/* End pointer */

	/* Odds and ends to allow compilation of inlines and drivers */
	unsigned char		ip_summed;	/* Driver fed us IP checksum */
	atomic_t		users;
	unsigned char		cloned;

	/* Click and probably the NodeOS are going to use these. */
	char			cb[96];
	union {
		unsigned char	*raw;
	} h;
	union {
		struct iphdr	*iph;
	} nh;

	/* These are new fields just for the oskit. */
	atomic_t		fdev_count;	/* reference count */
	oskit_bufio_t		*data_io;	/* COM buffer I/O pointer */

	/*
	 * The COM object that encapsulates sk_buff. Use this to map
	 * between the two.
	 */
	oskit_skbufio_t		ioi;
};

#ifndef __KERNEL__
/*
 ************************************************************************
 * Define some of the simpler operations as inlines. These are duplicates
 * of whats in the linux headers, but there just a few and they are trivial.
 */
static inline unsigned char *
skb_put(struct sk_buff *skb, unsigned int len)
{
	unsigned char *tmp = skb->tail;
	
	skb->tail += len;
	skb->len  += len;

	return tmp;
}

/* XXX this is not a standard Linux function */
static inline unsigned char *
skb_get(struct sk_buff *skb, unsigned int len)
{
	if (len > skb->len)
		return 0;

	skb->tail -= len;
	skb->len -= len;
	return skb->data;
}

static inline unsigned char *
skb_push(struct sk_buff *skb, unsigned int len)
{
	skb->data -= len;
	skb->len  += len;
	return skb->data;
}

static inline unsigned char *
skb_pull(struct sk_buff *skb, unsigned int len)
{	
	if (len > skb->len)
		return 0;

	skb->len -= len;
	return skb->data+=len;
}

static inline void
skb_reserve(struct sk_buff *skb, unsigned int len)
{
	skb->data += len;
	skb->tail += len;
}

static inline int
skb_headroom(struct sk_buff *skb)
{
	return skb->data - skb->head;
}

/* We leave this out since no one uses it but it tries to use skb->end. */
static inline int
skb_tailroom(struct sk_buff *skb)
{
	return skb->end - skb->tail;
}

/*
 ************************************************************************
 * The rest get pushed off to the object itself.
 */
static inline int
skb_cloned(struct sk_buff *skb)
{
	int	iscloned;

	oskit_skbufio_cloned(&skb->ioi, &iscloned);

	return iscloned;
}

#define GFP_ATOMIC	0

static inline struct sk_buff *
skb_clone(struct sk_buff *skb, int priority)
{
	oskit_skbufio_t		*new_skbufio;
	struct sk_buff		*new_skb;

	if (oskit_skbufio_clone(&skb->ioi, &new_skbufio))
		return 0;

	if (oskit_skbufio_metadata_map(new_skbufio,
				       SKB_VERSION_NUMBER, &new_skb))
		return 0;

	return new_skb;
}

static inline struct sk_buff *
skb_copy(struct sk_buff *skb, int priority)
{
	oskit_skbufio_t		*new_skbufio;
	struct sk_buff		*new_skb;

	if (oskit_skbufio_copy(&skb->ioi, 0, 0, &new_skbufio))
		return 0;

	if (oskit_skbufio_metadata_map(new_skbufio,
				       SKB_VERSION_NUMBER, &new_skb))
		return 0;

	return new_skb;
}

static inline void
__kfree_skb(struct sk_buff *skb)
{
	oskit_skbufio_release(&skb->ioi);
}

static inline struct sk_buff *
alloc_skb(unsigned int size, int priority)
{
	oskit_skbufio_t		*skbufio;
	struct sk_buff		*new_skb;
	
	if (oskit_skbufio_create(size, 0, 0, &skbufio))
		return 0;
	
	if (oskit_skbufio_metadata_map(skbufio, SKB_VERSION_NUMBER, &new_skb)) {
		oskit_skbufio_release(skbufio);
		return 0;
	}

	return new_skb;
}

static inline oskit_skbufio_t *
skb_skbufio(struct sk_buff *skb)
{
	return (oskit_skbufio_t *) &skb->ioi;
}
#endif
#endif /* _OSKIT_IO_SKBUFIO_H_ */
