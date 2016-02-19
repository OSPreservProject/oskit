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
 * Definitions of a simple wrapper that can be used to protect oskit_asyncio
 * interfaces.
 */

#include <oskit/io/asyncio.h>
#include <oskit/com/wrapper.h>
#include <oskit/c/stdlib.h>
#include <oskit/c/string.h>

typedef struct {
	oskit_asyncio_t	ioi;		/* COM interface being exported */
	int		count;		/* reference count */
	oskit_asyncio_t	*wrapped;	/* COM interface being wrapped */
	void		(*before)(void *);
	void		(*after)(void *);
	void		*cookie;
} asynciowrap_impl_t;

#define WRAPPERCALL(impl, call, args...) ({                      	\
        oskit_error_t __err;                                            \
        impl->before(impl->cookie);                                     \
        __err = oskit_asyncio_##call(impl->wrapped ,##args);          	\
        impl->after(impl->cookie);                                      \
        __err; })

#define WRAPPERCALL_NOARGS(impl, call) ({                        	\
        oskit_error_t __err;                                            \
        impl->before(impl->cookie);                                     \
        __err = oskit_asyncio_##call(impl->wrapped);                    \
        impl->after(impl->cookie);                                  	\
        __err; })


static OSKIT_COMDECL
asyncio_query(oskit_asyncio_t *f, const struct oskit_guid *iid,
	void **out_ihandle)
{
        asynciowrap_impl_t  *b = (asynciowrap_impl_t *)f;

	if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
            memcmp(iid, &oskit_asyncio_iid, sizeof(*iid)) == 0) {
                *out_ihandle = &b->ioi;
		b->before(b->cookie);
                ++b->count;
		b->after(b->cookie);
                return 0;
        }
        *out_ihandle = NULL;
        return OSKIT_E_NOINTERFACE;
}

static OSKIT_COMDECL_U
asyncio_addref(oskit_asyncio_t *f)
{
        asynciowrap_impl_t *b = (asynciowrap_impl_t *)f;
	unsigned newcount;

	b->before(b->cookie);
	newcount = ++b->count;
	b->after(b->cookie);
        return newcount;
}

static OSKIT_COMDECL_U
asyncio_release(oskit_asyncio_t *f)
{
        asynciowrap_impl_t *b = (asynciowrap_impl_t *)f;
        unsigned newcount;

	b->before(b->cookie);
        if ((newcount = --b->count) == 0) {
		oskit_asyncio_release(b->wrapped);
		b->after(b->cookie);
		free(b);
	} else
		b->after(b->cookie);
	return newcount;
}

/*******************************************************/
/******* Implementation of the oskit_asyncio_t if *******/
/*******************************************************/

static OSKIT_COMDECL
asyncio_poll(oskit_asyncio_t *f)
{
        asynciowrap_impl_t *si = (asynciowrap_impl_t *)f;
        oskit_error_t    err;

        err = WRAPPERCALL_NOARGS(si, poll);

        return err;
}

static OSKIT_COMDECL
asyncio_add_listener(oskit_asyncio_t *f, struct oskit_listener *l,
        oskit_s32_t mask)
{
        asynciowrap_impl_t *si = (asynciowrap_impl_t *)f;
        oskit_error_t    err;

        err = WRAPPERCALL(si, add_listener, l, mask);

        return err;
}

static OSKIT_COMDECL
asyncio_remove_listener(oskit_asyncio_t *f, struct oskit_listener *l0)
{
        asynciowrap_impl_t *si = (asynciowrap_impl_t *)f;
        oskit_error_t    err;

        err = WRAPPERCALL(si, remove_listener, l0);

        return err;
}
/*
 * return the number of bytes that can be read, basically ioctl(FIONREAD)
 */
static OSKIT_COMDECL
asyncio_readable(oskit_asyncio_t *f)
{
        asynciowrap_impl_t *si = (asynciowrap_impl_t *)f;
        oskit_error_t    err;

        err = WRAPPERCALL_NOARGS(si, readable);

        return err;
}

/***************************************************************************/
/*
 * vtables for interfaces exported by streams
 */
static struct oskit_asyncio_ops asyncioops =
{
        asyncio_query,
        asyncio_addref,
        asyncio_release,
        asyncio_poll,
        asyncio_add_listener,
        asyncio_remove_listener,
        asyncio_readable
};

/*
 * Wrap a asyncio_t in before/after brackets.
 */
oskit_error_t
oskit_wrap_asyncio(oskit_asyncio_t *in,
	void (*before)(void *), void (*after)(void *), void *cookie,
	oskit_asyncio_t **out)
{
        asynciowrap_impl_t *newsi = malloc(sizeof(*newsi));

        if (newsi == NULL)
                return OSKIT_ENOMEM;

	newsi->count = 1;
	newsi->ioi.ops = &asyncioops;
	newsi->wrapped = in;
	oskit_asyncio_addref(in);
	newsi->before = before;
	newsi->after = after;
	newsi->cookie = cookie;
	*out = &newsi->ioi;
	return 0;
}
