/*
 * Copyright (c) 2000 University of Utah and the Flux Group.
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
#include <string.h>
#include <stdlib.h>

#include <oskit/c/assert.h>
#include <oskit/dev/osenv.h>
#include <oskit/dev/osenv_mem.h>
#include <oskit/io/bufio.h>

/*
 * XXX hack: we don't OSKit/Unix users to have to include oskit_libkern
 * to get the expanded versions of the atomic routines, so we force them
 * to inline here.  Must be done before include of skbufio.h.
 */
#undef OSKIT_INLINE
#define OSKIT_INLINE static
#include <oskit/machine/atomic.h>

#include <oskit/io/skbufio.h>

/*
 * OSKit/Unix bufio/skbufio base implementation.
 *
 * Implements bufio/skbufios for OSKit/Unix.  Basically the same as the
 * linux/dev/skbuff.c implemenation, but doesn't implement all the standard
 * Linux skb_* interfaces used by device drivers, etc.  We do this to avoid
 * having to link in the oskit_linuxdev library in Unix mode.
 *
 * oskit_bufio_create just returns an skbufio.
 */

static inline struct sk_buff *
skbio_to_skbuff(oskit_skbufio_t *io)
{
	struct sk_buff *skb;

	assert(io != 0);

	skb = (struct sk_buff *)((void *)io - (int)&((struct sk_buff *)0)->ioi);

	assert(atomic_read(&(skb)->fdev_count) > 0);
	assert(skb->end - skb->head == skb->truesize);

	return skb;
} 

static OSKIT_COMDECL 
skbuff_io_query(oskit_skbufio_t *io, const oskit_iid_t *iid,
		void **out_ihandle) 
{
	struct sk_buff *skb;

	if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
	    memcmp(iid, &oskit_skbufio_iid, sizeof(*iid)) == 0 ||
	    memcmp(iid, &oskit_bufio_iid, sizeof(*iid)) == 0) {
		skb = skbio_to_skbuff(io);
		atomic_inc(&skb->fdev_count);

		*out_ihandle = &skb->ioi;
		return 0;
	}

	*out_ihandle = 0;
	return OSKIT_E_NOINTERFACE;
}

static OSKIT_COMDECL_U 
skbuff_io_addref(oskit_skbufio_t *io)
{
	struct sk_buff *skb = skbio_to_skbuff(io);

	/* XXX inc+read is not atomic */
	atomic_inc(&skb->fdev_count);

	return atomic_read(&skb->fdev_count);
}

static OSKIT_COMDECL_U 
skbuff_io_release(oskit_skbufio_t *io) 
{
	struct sk_buff *skb = skbio_to_skbuff(io);

	/*
	 * Decrement and read reference count atomically.
	 * Needed since this routine may be called at interrupt time.
	 */
	if (atomic_dec_and_test(&skb->fdev_count) != 0) {
		assert(skb->data_io == 0);
		oskit_skbufio_mem_free(skb, SKB_HDRSIZE + skb->truesize);
	}

	return atomic_read(&skb->fdev_count);
}

static OSKIT_COMDECL
skbuff_io_getsize(oskit_skbufio_t *io, oskit_off_t *out_size)
{
	struct sk_buff *skb = skbio_to_skbuff(io);

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
	struct sk_buff *skb = skbio_to_skbuff(io);

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
	struct sk_buff *skb = skbio_to_skbuff(io);

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
	struct sk_buff *skb = skbio_to_skbuff(io);

	if (offset + count > skb->len)
		return OSKIT_EINVAL;

	*out_addr = skb->data + offset;

	return 0;
}

static OSKIT_COMDECL 
skbuff_io_unmap(oskit_skbufio_t *io, void *addr,
		oskit_off_t offset, oskit_size_t count) 
{
	struct sk_buff *skb = skbio_to_skbuff(io);

	if (offset + count > skb->len)
		return OSKIT_EINVAL;

	return 0;
}

static OSKIT_COMDECL 
skbuff_io_wire(oskit_skbufio_t *io, oskit_addr_t *out_phys_addr,
	       oskit_off_t offset, oskit_size_t count)
{
	struct sk_buff *skb = skbio_to_skbuff(io);

	if (offset + count > skb->len)
		return OSKIT_EINVAL;

	return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL 
skbuff_io_unwire(oskit_skbufio_t *io, oskit_addr_t phys_addr,
		 oskit_off_t offset, oskit_size_t count)
{
	struct sk_buff *skb = skbio_to_skbuff(io);

	if (offset + count > skb->len)
		return OSKIT_EINVAL;

	return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
skbuff_io_copy(oskit_skbufio_t *io, oskit_off_t offset, oskit_size_t count,
	       oskit_skbufio_t **out_io)
{
	struct sk_buff *skb = skbio_to_skbuff(io);

	if (offset + count > skb->len)
		return OSKIT_EINVAL;

	return OSKIT_E_NOTIMPL;
}

/*
 * metadata_map:
 */
static OSKIT_COMDECL
skbuff_io_metadata_map(oskit_skbufio_t *io, oskit_u32_t version,
		       struct sk_buff **out_addr)
{
	struct sk_buff *skb = skbio_to_skbuff(io);

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
	return 0;
}

/*
 * Clone makes a complete copy of the given skbufio.
 * Also, there better not be an external bufio attached.
 */
static OSKIT_COMDECL
skbuff_io_clone(oskit_skbufio_t *io, oskit_skbufio_t **out_io)
{
	struct sk_buff *skb, *newskb;
	oskit_skbufio_t *newio;
	oskit_error_t err;

	skb = skbio_to_skbuff(io);
	assert(skb->data_io == 0);

	err = oskit_skbufio_create(skb->len, skb->data - skb->head,
				   skb->end - skb->tail, &newio);
	if (err != 0)
		return err;

	/* Copy the bytes */
	newskb = skbio_to_skbuff(newio);
	memcpy(newskb->head, skb->head, skb->end - skb->head);
	
	*out_io = newio;
	return 0;
}

/*
 * Cloned:
 */
static OSKIT_COMDECL
skbuff_io_cloned(oskit_skbufio_t *io, int *cloned)
{
	*cloned = 0;
	return 0;
}

static struct oskit_skbufio_ops skbuff_io_ops = {
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
	unsigned int hdrsize, bufsize;

	hdrsize = SKB_HDRSIZE;
	bufsize = size + hroom + troom;

	skb = oskit_skbufio_mem_alloc(hdrsize+bufsize, OSENV_NONBLOCKING,
				      &bufsize);
	if (skb == 0)
		return OSKIT_E_OUTOFMEMORY;

	skb->truesize = bufsize - hdrsize;
	skb->ioi.ops = &skbuff_io_ops;
	skb->dev = NULL;
	skb->len = 0;
	skb->prev = skb->next = NULL;
	skb->list = NULL;
	skb->head = (char *)skb + hdrsize;
	skb->data = skb->tail = skb->head;
	skb->end = skb->head + skb->truesize;
	atomic_set(&skb->fdev_count, 1);
	skb->data_io = NULL;
	skb->cloned = 0;
	atomic_set(&skb->users, 1); 

	if (hroom != 0)
		skb_reserve(skb, hroom);
	if (size != 0)
		skb_put(skb, size);

	*out_skbufio = &skb->ioi;

	return 0;
}

oskit_bufio_t *
oskit_bufio_create(oskit_size_t size) 
{
	oskit_skbufio_t *skbio;
	oskit_error_t err;
	
	err = oskit_skbufio_create(size, 0, 0, &skbio);
	return err ? 0 : (oskit_bufio_t *)skbio;
}
