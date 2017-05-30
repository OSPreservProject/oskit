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
 * Definitions of a simple wrapper that can be used to protect oskit_stream
 * interfaces.
 */

#include <oskit/com/stream.h>
#include <oskit/com/wrapper.h>
#include <oskit/c/stdlib.h>
#include <oskit/c/string.h>

typedef struct {
	oskit_stream_t	ioi;		/* COM interface being exported */
	int		count;		/* reference count */
	oskit_stream_t	*wrapped;	/* COM interface being wrapped */
	void		(*before)(void *);
	void		(*after)(void *);
	void		*cookie;
} streamwrap_impl_t;

#define WRAPPERCALL(impl, call, args...) ({                      	\
        oskit_error_t __err;                                            \
        impl->before(impl->cookie);                                     \
        __err = oskit_stream_##call(impl->wrapped ,##args);          	\
        impl->after(impl->cookie);                                      \
        __err; })

#define WRAPPERCALL_NOARGS(impl, call) ({                        	\
        oskit_error_t __err;                                            \
        impl->before(impl->cookie);                                     \
        __err = oskit_stream_##call(impl->wrapped);                     \
        impl->after(impl->cookie);                                    	\
        __err; })


static OSKIT_COMDECL
stream_query(oskit_stream_t *f, const struct oskit_guid *iid,
	void **out_ihandle)
{
        streamwrap_impl_t  *b = (streamwrap_impl_t *)f;

	if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
            memcmp(iid, &oskit_stream_iid, sizeof(*iid)) == 0) {
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
stream_addref(oskit_stream_t *f)
{
        streamwrap_impl_t *b = (streamwrap_impl_t *)f;
        unsigned newcount;

	b->before(b->cookie);
	newcount = ++b->count;
	b->after(b->cookie);
	return newcount;
}

static OSKIT_COMDECL_U
stream_release(oskit_stream_t *f)
{
        streamwrap_impl_t *b = (streamwrap_impl_t *)f;
        unsigned newcount;

	b->before(b->cookie);
        if ((newcount = --b->count) == 0) {
		oskit_stream_release(b->wrapped);
		b->after(b->cookie);
		free(b);
	} else
		b->after(b->cookie);
	return newcount;
}

/*******************************************************/
/******* Implementation of the oskit_stream_t if *******/
/*******************************************************/

static OSKIT_COMDECL
stream_read(oskit_stream_t *f, void *buf, oskit_u32_t len,
                                oskit_u32_t *out_actual)
{
        streamwrap_impl_t *s = (streamwrap_impl_t *)f;
        oskit_error_t    err;

        err = WRAPPERCALL(s, read, buf, len, out_actual);

        return err;
}

static OSKIT_COMDECL
stream_write(oskit_stream_t *f, const void *buf,
                                 oskit_u32_t len, oskit_u32_t *out_actual)
{
        streamwrap_impl_t *s = (streamwrap_impl_t *)f;
        oskit_error_t    err;

        err = WRAPPERCALL(s, write, buf, len, out_actual);

        return err;
}

static OSKIT_COMDECL
stream_seek(oskit_stream_t *f, oskit_s64_t ofs,
        oskit_seek_t whence, oskit_u64_t *out_newpos)
{
        streamwrap_impl_t *s = (streamwrap_impl_t *)f;
        oskit_error_t    err;

        err = WRAPPERCALL(s, seek, ofs, whence, out_newpos);

        return err;
}

static OSKIT_COMDECL
stream_setsize(oskit_stream_t *f, oskit_u64_t new_size)
{
        streamwrap_impl_t *s = (streamwrap_impl_t *)f;
        oskit_error_t    err;

        err = WRAPPERCALL(s, setsize, new_size);

        return err;
}

static OSKIT_COMDECL
stream_copyto(oskit_stream_t *f, oskit_stream_t *dst,
        oskit_u64_t size,
        oskit_u64_t *out_read,
        oskit_u64_t *out_written)
{
        streamwrap_impl_t *s = (streamwrap_impl_t *)f;
        oskit_error_t    err;

        err = WRAPPERCALL(s, copyto,
                          dst, size, out_read, out_written);

        return err;
}

static OSKIT_COMDECL
stream_commit(oskit_stream_t *f, oskit_u32_t commit_flags)
{
        streamwrap_impl_t *s = (streamwrap_impl_t *)f;
        oskit_error_t    err;

        err = WRAPPERCALL(s, commit, commit_flags);

        return err;
}

static OSKIT_COMDECL
stream_revert(oskit_stream_t *f)
{
        streamwrap_impl_t *s = (streamwrap_impl_t *)f;
        oskit_error_t    err;

        err = WRAPPERCALL_NOARGS(s, revert);

        return err;
}

static OSKIT_COMDECL
stream_lock_region(oskit_stream_t *f,
        oskit_u64_t offset, oskit_u64_t size, oskit_u32_t lock_type)
{
        streamwrap_impl_t *s = (streamwrap_impl_t *)f;
        oskit_error_t    err;

        err = WRAPPERCALL(s, lockregion, offset, size, lock_type);

        return err;
}

static OSKIT_COMDECL
stream_unlock_region(oskit_stream_t *f,
        oskit_u64_t offset, oskit_u64_t size, oskit_u32_t lock_type)
{
        streamwrap_impl_t *s = (streamwrap_impl_t *)f;
        oskit_error_t    err;

        err = WRAPPERCALL(s, unlockregion, offset, size, lock_type);

        return err;
}

static OSKIT_COMDECL
stream_stat(oskit_stream_t *f, oskit_stream_stat_t *out_stat,
        oskit_u32_t stat_flags)
{
        streamwrap_impl_t *s = (streamwrap_impl_t *)f;
        oskit_error_t    err;

        err = WRAPPERCALL(s, stat, out_stat, stat_flags);

        return err;
}

static OSKIT_COMDECL
stream_clone(oskit_stream_t *f, oskit_stream_t **out_stream)
{
        streamwrap_impl_t *s = (streamwrap_impl_t *)f;
        oskit_error_t    err;
	oskit_stream_t *out;

        err = WRAPPERCALL(s, clone, &out);

	if (err == 0) {
		err = oskit_wrap_stream(out, s->before, s->after, s->cookie,
			out_stream);
		oskit_stream_release(out);
	}
        return err;
}

/***************************************************************************/
/*
 * vtables for interfaces exported by streams
 */
static struct oskit_stream_ops streamops =
{
        stream_query,
        stream_addref,
        stream_release,
        stream_read,
        stream_write,
        stream_seek,
        stream_setsize,
        stream_copyto,
        stream_commit,
        stream_revert,
        stream_lock_region,
        stream_unlock_region,
        stream_stat,
        stream_clone
};

/*
 * Wrap a stream_t in before/after brackets.
 */
oskit_error_t
oskit_wrap_stream(oskit_stream_t *in,
	void (*before)(void *), void (*after)(void *), void *cookie,
	oskit_stream_t **out)
{
        streamwrap_impl_t *newsi = malloc(sizeof(*newsi));

        if (newsi == NULL)
                return OSKIT_ENOMEM;

	newsi->count = 1;
	newsi->ioi.ops = &streamops;
	newsi->wrapped = in;
	oskit_stream_addref(in);
	newsi->before = before;
	newsi->after = after;
	newsi->cookie = cookie;
	*out = &newsi->ioi;
	return 0;
}
