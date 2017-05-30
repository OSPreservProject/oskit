/*
 * Copyright (c) 1996-2000 The University of Utah and the Flux Group.
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
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 */
#include <oskit/dev/net.h>
#include <oskit/io/netio.h>

#include <oskit/c/assert.h>

#include "net.h"
#include "net_debug.h"
#include "osenv.h"

#include <linux/skbuff.h>		/* struct sk_buff */
#include <linux/string.h>		/* memcmp */

/* forward decls */
static struct oskit_skbufio_ops linux_skbuff_io_ops;
static void dealloc_skb(struct sk_buff *skb);

/* No longer exists? */
#define IS_SKB(skb)

/*
 * Allocate an sk_buff with SIZE bytes of data space and fill in a few
 * fields.
 *
 * This is adapted from the Mach one so we set skb->len to size
 * instead of zero like the Linux version does.
 * It is not clear to me why this is but I think it has to do with Linux
 * using the skb_put, etc macros to set len.
 * Also, the Mach one called kmalloc with GFP_KERNEL instead of ATOMIC for
 * some unknown reason.
 *
 * Compare
 *	Linux	net/core/skbuff.c
 *	Mach4	linux_net.c
 */
struct sk_buff *
alloc_skb(unsigned int size, int priority)
{
	struct sk_buff *skb;
	unsigned int bufsize, hdrsize;

	hdrsize = SKB_HDRSIZE;

	assert(priority == GFP_ATOMIC);
	skb = oskit_skbufio_mem_alloc((oskit_size_t)(hdrsize + size),
				      OSENV_NONBLOCKING, &bufsize);
	if (skb) {
		skb->truesize = bufsize;
		skb->ioi.ops = &linux_skbuff_io_ops;
		skb->dev = NULL;
		skb->len = 0;
	
		skb->prev = skb->next = NULL;
		skb->list = NULL;

		skb->head = (char *)skb + hdrsize;
		skb->data = skb->tail = skb->head;
		skb->end = (char *)skb + bufsize;

		atomic_set(&skb->fdev_count, 1);
		skb->data_io = NULL;

		skb->cloned = 0;
		atomic_set(&skb->users, 1); 
	}
	return skb;
}

/*
 * Wrapper for alloc_skb.
 *
 * The Linux version allocates 16 bytes too much for some unknown
 * reason.  The Mach version doesn't but reverses the sense of
 * alloc_skb and dev_alloc_skb.
 *
 * Compare
 *	Linux	net/core/skbuff.c
 *	Mach4	linux_net.c
 */
struct sk_buff *
dev_alloc_skb(unsigned int size)
{
        struct sk_buff *skb;

	/* Allocate and extra 16 bytes at the beginning for an
	 * ethernet header.
	 */
        skb = alloc_skb(size + 16, GFP_ATOMIC);
        if (skb)
                skb_reserve(skb, 16);

        return skb;
}

/*
 * Free the sk_buff SKB.
 *
 * The Linux version does some lock and refcount checking but,
 * like the Mach one, we don't.  And like before, I don't know why.
 * 
 * Compare
 *	Linux	net/core/skbuff.c
 *	Mach4	linux_net.c
 */
void
__kfree_skb(struct sk_buff *skb)
{
	if (atomic_dec_and_test(&skb->fdev_count) != 0)
		dealloc_skb(skb);
}

static void
dealloc_skb(struct sk_buff *skb)
{
	/*
	 * If we've got a data_io pointer, then we need to unmap the 
	 * buffer and release our handle to the bufio object.
	 */
	if (skb->data_io) {
		oskit_bufio_unmap(skb->data_io, skb->data, 0, skb->len);
		oskit_bufio_release(skb->data_io);
		skb->data_io = NULL;
	}

	oskit_skbufio_mem_free(skb, skb->truesize);
}

#if 0
/*
 * Add data to an sk_buff.
 *
 * Linux	net/core/skbuff.c
 */
unsigned char *
skb_put(struct sk_buff *skb, unsigned int len)
{
        unsigned char *tmp = skb->tail;

        IS_SKB(skb);
        skb->tail += len;
        skb->len += len;
        IS_SKB(skb);

        if (skb->tail > skb->end)
                panic("skput:over: %p:%d", __builtin_return_address(0),len);

        return tmp;
}

/*
 * preallocate some space at the beginning of the sk_buff buffer. 
 *
 * Linux	net/core/skbuff.c
 */
void 
skb_reserve(struct sk_buff *skb, unsigned int len)
{
        IS_SKB(skb);
        skb->data += len;
        skb->tail += len;

        if (skb->tail > skb->end)
                panic("sk_res: over");
        if (skb->data < skb->head)
                panic("sk_res: under");
        IS_SKB(skb);
}
#endif

/*
 * skbuff COM interface definitions.
 */
#define GET_SKB_FROM_BUF_IO(io, skb) { \
	/* our bufio ops struct is at the end of the skbuff. */ \
	(skb) = (struct sk_buff *)(((char *)(io)) - \
		(sizeof(struct sk_buff) - sizeof(struct oskit_skbufio))); \
	assert((skb) != 0); \
	assert(atomic_read(&(skb)->fdev_count) > 0); \
} 

static OSKIT_COMDECL 
skbuff_io_query(oskit_skbufio_t *io, const oskit_iid_t *iid,
			    void **out_ihandle) 
{
	struct sk_buff *skb;

	
	GET_SKB_FROM_BUF_IO(io, skb);

        if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
            memcmp(iid, &oskit_skbufio_iid, sizeof(*iid)) == 0 ||
            memcmp(iid, &oskit_bufio_iid, sizeof(*iid)) == 0) {
                *out_ihandle = &skb->ioi;
		atomic_inc(&skb->fdev_count);

                return 0;
        }

        *out_ihandle = NULL;
        return OSKIT_E_NOINTERFACE;
}

static OSKIT_COMDECL_U 
skbuff_io_addref(oskit_skbufio_t *io)
{
	struct sk_buff *skb;

	GET_SKB_FROM_BUF_IO(io, skb);

	/* XXX not atomic */
	atomic_inc(&skb->fdev_count);
        return atomic_read(&skb->fdev_count);
}

static OSKIT_COMDECL_U 
skbuff_io_release(oskit_skbufio_t *io) 
{
	struct sk_buff *skb;

	GET_SKB_FROM_BUF_IO(io, skb);

	/*
	 * Decrement and read reference count atomically.
	 * Needed since this routine will be called by the network transmit
	 * done interrupt handler.
	 */
	if (atomic_dec_and_test(&skb->fdev_count) != 0) {
                /* ref count = 0, need to free! */

		/* 
		 * We ignore the parameter to kfree_skb, since it only
		 * has to do with freeing the struct sock in an sk_buff,
		 * which we don't support anyway. =)
		 */
		dealloc_skb(skb);
		return 0;
        }

	return atomic_read(&skb->fdev_count);
}

static OSKIT_COMDECL
skbuff_io_getsize(oskit_skbufio_t *io, oskit_off_t *out_size)
{
	struct sk_buff *skb;

	GET_SKB_FROM_BUF_IO(io, skb);

	*out_size = skb->len;
	return 0;
}

static OSKIT_COMDECL
skbuff_io_setsize(oskit_skbufio_t *io, oskit_off_t new_size)
{
	return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL 
skbuff_io_read(oskit_skbufio_t *io, void *dest,
	       oskit_off_t offset, oskit_size_t count,
	       oskit_size_t *out_actual) 
{
	struct sk_buff *skb;

	GET_SKB_FROM_BUF_IO(io, skb);

	if (offset + count > skb->len)
		count = skb->len - offset;

        memcpy(dest, skb->data + offset, count);
	
	*out_actual = count;

        return (0);
}

static OSKIT_COMDECL 
skbuff_io_write(oskit_skbufio_t *io, const void *src,
		oskit_off_t offset, oskit_size_t count,
		oskit_size_t *out_actual)
{
	struct sk_buff *skb;

	GET_SKB_FROM_BUF_IO(io, skb);

	if (offset + count > skb->len)
		count = skb->len - offset;

        memcpy(skb->data + offset, src, count);

	*out_actual = count;

        return 0;
}

static OSKIT_COMDECL 
skbuff_io_map(oskit_skbufio_t *io, void **out_addr,
	      oskit_off_t offset, oskit_size_t count)
{
	struct sk_buff *skb;

	GET_SKB_FROM_BUF_IO(io, skb);

	if (offset + count > skb->len)
		return OSKIT_EINVAL;

 	*out_addr = skb->data + offset;

        return 0;
}

static OSKIT_COMDECL 
skbuff_io_unmap(oskit_skbufio_t *io, void *addr,
		oskit_off_t offset, oskit_size_t count) 
{
	struct sk_buff *skb;

	GET_SKB_FROM_BUF_IO(io, skb);

	if (offset + count > skb->len)
		return OSKIT_EINVAL;

	return 0;
}

static OSKIT_COMDECL 
skbuff_io_wire(oskit_skbufio_t *io, oskit_addr_t *out_phys_addr,
	       oskit_off_t offset, oskit_size_t count)
{
	struct sk_buff *skb;

	GET_SKB_FROM_BUF_IO(io, skb);

	if (offset + count > skb->len)
		return OSKIT_EINVAL;

	return OSKIT_E_NOTIMPL; /*XXX*/
}

static OSKIT_COMDECL 
skbuff_io_unwire(oskit_skbufio_t *io, oskit_addr_t phys_addr,
	         oskit_off_t offset, oskit_size_t count)
{
	struct sk_buff *skb;

	GET_SKB_FROM_BUF_IO(io, skb);

	if (offset + count > skb->len)
		return OSKIT_EINVAL;

	return OSKIT_E_NOTIMPL; /*XXX*/
}

static OSKIT_COMDECL
skbuff_io_copy(oskit_skbufio_t *io, oskit_off_t offset,
	       oskit_size_t count, oskit_skbufio_t **out_io)
{
	struct sk_buff *skb;

	GET_SKB_FROM_BUF_IO(io, skb);

	if (offset + count > skb->len)
		return OSKIT_EINVAL;

	return OSKIT_E_NOTIMPL;
}

/*
 * metadata_map:
 */
static OSKIT_COMDECL
skbuff_io_metadata_map(oskit_skbufio_t *io,
		       oskit_u32_t version, struct sk_buff **out_addr)
{
	struct sk_buff *skb;

	GET_SKB_FROM_BUF_IO(io, skb);

	if (version != SKB_VERSION_NUMBER)
		return OSKIT_E_INVALIDARG;

 	*out_addr = skb;

	return 0;
}

/*
 * metadata_unmap:
 */
static OSKIT_COMDECL
skbuff_io_metadata_unmap(oskit_skbufio_t *io, struct sk_buff *addr)
{
	struct sk_buff *skb;

	GET_SKB_FROM_BUF_IO(io, skb);

	return 0;
}

/*
 * Clone: Clone just makes a complete copy ... Also, there better not
 * be an external bufio attached.
 */
static OSKIT_COMDECL
skbuff_io_clone(oskit_skbufio_t *io, oskit_skbufio_t **out_io)
{
	struct sk_buff *skb, *newskb;

	GET_SKB_FROM_BUF_IO(io, skb);
	assert(skb->data_io == 0);

	newskb = alloc_skb(skb->end - skb->head, GFP_ATOMIC);
        if (!newskb)
		return OSKIT_E_OUTOFMEMORY;

	/* Set the data pointer */
	skb_reserve(newskb, skb->data - skb->head);
	/* Set the tail pointer and length */
	skb_put(newskb, skb->len);
	/* Copy the bytes */
	memcpy(newskb->head, skb->head, skb->end - skb->head);
	
	*out_io = &newskb->ioi;
	return 0;
}

/*
 * Cloned:
 */
static OSKIT_COMDECL
skbuff_io_cloned(oskit_skbufio_t *io, int *cloned)
{
	struct sk_buff *skb;

	GET_SKB_FROM_BUF_IO(io, skb);

	*cloned = 0;
	
	return 0;
}

/*
 * Hook to allow us to determine when an incoming bufio is actually
 * on of our own SKB/BUFIO widgets. In this case, we don't want to
 * clone it, but just bump the reference count and use the SKB directly.
 */
struct sk_buff *
skbuff_is_skbufio(oskit_bufio_t *io)
{
	if (((oskit_skbufio_t *)io)->ops == &linux_skbuff_io_ops) {
		struct sk_buff *skb;
		
		GET_SKB_FROM_BUF_IO(io, skb);
		atomic_inc(&skb->fdev_count);
		return skb;
	}
	return 0;
}

static struct oskit_skbufio_ops linux_skbuff_io_ops = {
        skbuff_io_query, 
	skbuff_io_addref, 
	skbuff_io_release,
        (void *) 0,
	skbuff_io_read, 
	skbuff_io_write, 
	skbuff_io_getsize,
	skbuff_io_setsize,
        skbuff_io_map, 
	skbuff_io_unmap, 
	skbuff_io_wire, 
	skbuff_io_unwire,
	skbuff_io_copy,
	skbuff_io_metadata_map,
	skbuff_io_metadata_unmap,
	skbuff_io_clone,
	skbuff_io_cloned,
};

oskit_error_t
oskit_skbufio_create(oskit_size_t size, oskit_size_t hroom, oskit_size_t troom,
		     oskit_skbufio_t **out_skbufio)
{
	struct sk_buff *skb;
	unsigned int bsize;

	bsize = size + hroom + troom;
	skb = alloc_skb(bsize, GFP_ATOMIC);
	if (!skb)
		return OSKIT_E_OUTOFMEMORY;

	if (hroom != 0)
		skb_reserve(skb, hroom);
	if (size != 0)
		skb_put(skb, size);
	assert(skb_tailroom(skb) >= troom);

	*out_skbufio = &skb->ioi;

	return 0;
}
