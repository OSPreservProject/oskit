/*
 * Copyright (c) 1997-1999 University of Utah and the Flux Group.
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
 * BSD/MBuf implementation of buffer routines.
 *
 * these are used on output, that is, if BSD passed an mbuf to the
 * device driver to be sent - we will act as a oskit_bufio_t, but
 * draw the data to be copied out from BSD mbufs.
 */
#include <oskit/dev/dev.h>
#include <oskit/io/bufio.h>
#include <oskit/io/bufiovec.h>
#include <oskit/c/string.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include "net/if.h"
#include "net/route.h"
#include "netinet/in.h"
#include "netinet/if_ether.h"

#define VERBOSITY 0
#include "mbuf_buf_io.h"

typedef struct mbuf_bufio {
        oskit_bufio_t           ioi;    /* COM I/O Interface for bufio */
        oskit_bufiovec_t        iov;    /* COM I/O Interface for bufiovec */
        unsigned        count;          /* reference count */

        struct mbuf     *m;
        oskit_size_t    mlen;
} mbuf_bufio_t;

/* 
 * Query this buffer I/O object for its interfaces.
 */
static OSKIT_COMDECL
query(mbuf_bufio_t *b, const oskit_iid_t *iid, void **out_ihandle) 
{
	if (b == NULL)
		osenv_panic("%s:%d: null bufio_t", __FILE__, __LINE__);
	if (b->count == 0)
		osenv_panic("%s:%d: bad count", __FILE__, __LINE__);

	if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
	    memcmp(iid, &oskit_bufio_iid, sizeof(*iid)) == 0) {
		*out_ihandle = &b->ioi;
		++b->count;
		
		return 0;
	}

	if (memcmp(iid, &oskit_bufiovec_iid, sizeof(*iid)) == 0) {
		*out_ihandle = &b->iov;
		++b->count;
		
		return 0;
	}

	*out_ihandle = NULL;
	return OSKIT_E_NOINTERFACE;
}

static OSKIT_COMDECL
bufio_query(oskit_bufio_t *io, const oskit_iid_t *iid, void **out_ihandle) 
{
	mbuf_bufio_t *b = (mbuf_bufio_t *)io;
	return query(b, iid, out_ihandle);
}

static OSKIT_COMDECL
bufiovec_query(oskit_bufiovec_t *io, const oskit_iid_t *iid, void **out) 
{
	mbuf_bufio_t *b = (mbuf_bufio_t *)(io - 1);
	return query(b, iid, out);
}

/*
 * Clone a reference to a device's block I/O interface.
 */
static OSKIT_COMDECL_U
addref(mbuf_bufio_t *b)
{
	if (b == NULL)
                osenv_panic("%s:%d: null bufio_t", __FILE__, __LINE__);
        if (b->count == 0)
                osenv_panic("%s:%d: bad count", __FILE__, __LINE__);

	return ++b->count;
}

static OSKIT_COMDECL_U
bufio_addref(oskit_bufio_t *io)
{
	mbuf_bufio_t *b = (mbuf_bufio_t *)io;   
	return addref(b);
}

static OSKIT_COMDECL_U
bufiovec_addref(oskit_bufiovec_t *io)
{
	mbuf_bufio_t *b = (mbuf_bufio_t *)(io - 1);
	return addref(b);
}

static OSKIT_COMDECL_U 
release(mbuf_bufio_t *b)
{
	unsigned newcount = 0;

        if (b == NULL)
                osenv_panic("%s:%d: null bufio_t", __FILE__, __LINE__);
        if (b->count == 0)
                osenv_panic("%s:%d: bad count", __FILE__, __LINE__);

	if ((newcount = --b->count) == 0) {
		/* ref count is 0, free ourselves. */
		m_freem(b->m);

		osenv_mem_free(b, 0, sizeof(*b));
	}

	return newcount;
}

static OSKIT_COMDECL_U
bufio_release(oskit_bufio_t *io)
{
	mbuf_bufio_t *b = (mbuf_bufio_t *)io;   
	return release(b);
}

static OSKIT_COMDECL_U
bufiovec_release(oskit_bufiovec_t *io)
{
	mbuf_bufio_t *b = (mbuf_bufio_t *)(io - 1);
	return release(b);
}

/*
 * Copy data from a user buffer into kernel's address space.
 */
static OSKIT_COMDECL
bufio_read(oskit_bufio_t *io, void *dest, oskit_off_t offset, 
	oskit_size_t count, oskit_size_t *actual)
{
	/*
	 * copy count bytes from offset to dest
	 */
	struct mbuf *m0 = ((mbuf_bufio_t *)io)->m;
	int mpos = 0;

#if VERBOSITY > 0
	osenv_log(OSENV_LOG_INFO, __FUNCTION__
		"(%p,%p,%d,%d) called\n", io, dest, offset, count);
#endif

	/* XXX - we don't even consider the case 
	 * that our mbuf might run out of bytes 
	 */
	*actual = count;

	/* find the mbuf to start with */
	while (offset > mpos + m0->m_len) {
		mpos += m0->m_len;
		m0 = m0->m_next;
	}
		
	while (count > 0) {
		/* copyout */
		int start = offset - mpos;
		int avail = m0->m_len - start;
		int bytes = avail < count ? avail : count;

		memcpy(dest, mtod(m0, caddr_t) + start, bytes);
#if VERBOSITY > 1
		hexdumpb(0, dest, bytes);
#endif
		dest += bytes;
		offset += bytes;
		count -= bytes;

		/* if there are bytes left to copy, advance to next mbuf */
		if (count > 0) {
			mpos += m0->m_len;
			m0 = m0->m_next;
			if (!m0)
				break;
		}
	}
	return 0;
}


/*
 * Copy data from kernel address space to a user buffer.
 */
static OSKIT_COMDECL
bufio_write(oskit_bufio_t *io, const void *src, 
	oskit_off_t offset, oskit_size_t count, oskit_size_t *actual) 
{
#if 1
	osenv_log(OSENV_LOG_ALERT, __FILE__":"__FUNCTION__
		" called - that shouldn't happen.\n");
	return OSKIT_E_NOTIMPL;
#else
	struct fdev_buf *b;

	b = (struct fdev_buf *)io;

        if (b == NULL)
                osenv_panic("%s:%d: null bufio_t", __FILE__, __LINE__);
        if (b->count == 0)
                osenv_panic("%s:%d: bad count", __FILE__, __LINE__);

	if (offset + count > b->size)
		return 1;	/* XXX appropriate error? */

	memcpy((void *)(b->buf + offset), src, count);

	return 0;
#endif
}

static OSKIT_COMDECL
bufio_getsize(oskit_bufio_t *io, oskit_off_t *out_size)
{
	mbuf_bufio_t *b = (mbuf_bufio_t *)io;
	*out_size = (oskit_off_t)b->mlen;
	return 0;
}

static OSKIT_COMDECL
bufio_setsize(oskit_bufio_t *io, oskit_off_t new_size)
{
	return OSKIT_E_NOTIMPL;
}

/*
 * we support it as best as we can...
 *
 * if it fails, the driver will 
 * call copyin when it needs the data from the mbufs
 */
static OSKIT_COMDECL
bufio_map(oskit_bufio_t *io, void **dest, 
    oskit_off_t offset, oskit_size_t count)
{
	struct mbuf *m = ((mbuf_bufio_t *)io)->m;
	int 	m_off = 0;

#ifndef NO_FAST_REJECT
	/* 
	 * this is what currently happens: it only makes sense to check
	 * if the mbuf chain has only one mbuf in it - since the device
	 * driver will want to map the whole thing in.
	 *
	 * even in the future it would be safe to leave that so as map
	 * is ``voluntary'' anyway.
	 */
	if (m->m_next)
		return OSKIT_E_NOTIMPL;
#endif
	/*
	 * go through the list of mbufs, trying to find one 
	 * that contains the requested range 
	 */
	while (m && m_off <= offset) {
		if (offset + count <= m_off + m->m_len)
			return *dest = m->m_data + offset - m_off, 0;

		m_off += m->m_len;
		m = m->m_next;	
	}
	return OSKIT_E_NOTIMPL;	/* XXX appropriate error? */
}

/*
 * convert this mbuf chain in a series of iovec's, provided by the caller
 */
static OSKIT_COMDECL_U
bufiovec_map(oskit_bufiovec_t *io, oskit_iovec_t *vec, oskit_u32_t veclen)
{
	struct mbuf *m = ((mbuf_bufio_t *)(io - 1))->m;

	int	n = 0;
	while (vec && m && n <= veclen) {
	    vec->iov_base = m->m_data;
	    vec->iov_len = m->m_len;
	    m = m->m_next;
	    vec++;
	    n++;
	}
	if (m)
	    return OSKIT_ENOBUFS;	/* XXX: No buffer space available */
	return n;
}

/* 
 * XXX These should probably do checking on their inputs so we could 
 * catch bugs in the code that calls them.
 */

static OSKIT_COMDECL
bufio_unmap(oskit_bufio_t *io, void *addr, 
      oskit_off_t offset, oskit_size_t count)
{
	return 0;
}

static OSKIT_COMDECL
bufio_wire(oskit_bufio_t *io, oskit_addr_t *out_phys_addr,
     oskit_off_t offset, oskit_size_t count)
{
	return 0;
}

static OSKIT_COMDECL
bufio_unwire(oskit_bufio_t *io, oskit_addr_t phys_addr,
       oskit_off_t offset, oskit_size_t count)
{
	return 0;
}

/*
 * vtable for bufio operations
 */
static struct oskit_bufio_ops mbuf_bio_ops = {
	bufio_query, 
	bufio_addref, 
	bufio_release,
	NULL,			/* reserved */
	bufio_read,
	bufio_write,
	bufio_getsize, 
	bufio_setsize, 
	bufio_map, 
	bufio_unmap, 
	bufio_wire, 
	bufio_unwire
};

/*
 * vtable for bufiovec operations
 */
static struct oskit_bufiovec_ops mbuf_biovec_ops = {
	bufiovec_query, 
	bufiovec_addref, 
	bufiovec_release,
	bufiovec_map
};

/*
 * create a bufio_t from an mbuf chain, and return length in *mlen
 */
oskit_bufio_t *
mbuf_bufio_create_instance(struct mbuf *m, oskit_size_t *mlen)
{
	mbuf_bufio_t *b;
	struct mbuf *m0 = m, *next_m0;

	/* COM specific part */
	b = (mbuf_bufio_t *)osenv_mem_alloc(sizeof(*b), 0, 0);
	if (b == NULL)
		return NULL;
	memset(b, 0, sizeof(*b));

	b->count = 1;
	b->ioi.ops = &mbuf_bio_ops;
	b->iov.ops = &mbuf_biovec_ops;

	/* mbuf_bufio_t specific part */
	b->m = m;
	b->mlen = 0;

/* for debugging */
#define PRINT_MBUFS 0

	/* walk through list of mbuf, and add them up */
	do {
#if PRINT_MBUFS
		printf("+%d ", m0->m_len);
#endif
		b->mlen += m0->m_len;
		next_m0 = m0->m_next;
	} while ((m0 = next_m0) != NULL);
#if PRINT_MBUFS
	printf(" = %d\n", b->mlen);
#endif
	if (mlen)
	    *mlen = b->mlen;
	return &b->ioi;
}
