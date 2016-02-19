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
 * Definitions for a COM object containing and wrapping the interfaces
 * contained in our character TTY implementation
 */

#include <oskit/com.h>
#include <oskit/com/wrapper.h>
#include <oskit/c/stdlib.h>
#include <oskit/c/stdio.h>
#include <oskit/c/string.h>
#include <oskit/io/asyncio.h>
#include <oskit/io/ttystream.h>
#include <oskit/dev/tty.h>

/*
 * Wrapped up ttystream interfaces.
 */
typedef struct {
	oskit_ttystream_t	ios;		/* COM I/O interface Nr. 1 */
	oskit_asyncio_t		ioa;		/* COM I/O interface Nr. 2 */
	unsigned		count;		/* reference count */
	oskit_ttystream_t	*w_ios;		/* COM I/O interface Nr. 1 */
	oskit_asyncio_t		*w_ioa;		/* COM I/O interface Nr. 2 */
	void			(*before)(void *);
	void			(*after)(void *);
	void			*cookie;
} oskit_ttyimpl_t;

#define WRAPPERCALL(impl, ops, iface, call, args...) ({			      \
	oskit_error_t __err;						      \
	impl->before(impl->cookie);					      \
	__err = oskit_##iface##_##call(impl->w_##ops ,##args);        	      \
	impl->after(impl->cookie);					      \
	__err; })

#define WRAPPERCALL_NOARGS(impl, ops, iface, call) ({			      \
	oskit_error_t __err;						      \
	impl->before(impl->cookie);					      \
	__err = oskit_##iface##_##call(impl->w_##ops);	      	      	      \
	impl->after(impl->cookie);					      \
	__err; })

/*
 * implementations of query, addref and release dealing with
 * different COM interfaces.
 */

/*
 * query
 */
static OSKIT_COMDECL
query(oskit_ttyimpl_t *b, const struct oskit_guid *iid, void **out_ihandle)
{
	oskit_error_t rc = 0;

	b->before(b->cookie);
        if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
	    memcmp(iid, &oskit_stream_iid, sizeof(*iid)) == 0 ||
            memcmp(iid, &oskit_ttystream_iid, sizeof(*iid)) == 0) {
		*out_ihandle = &b->ios;
		++b->count;
		b->after(b->cookie);
		return 0;
        }

        if (memcmp(iid, &oskit_asyncio_iid, sizeof(*iid)) == 0) {
		if (b->w_ioa == 0) 
			rc = oskit_ttystream_query(b->w_ios, 
				&oskit_asyncio_iid, (void **)&b->w_ioa);
		if (rc == 0) {
			*out_ihandle = &b->ioa;
			++b->count;
			b->after(b->cookie);
			return 0;
		}
	}
	b->after(b->cookie);

        *out_ihandle = NULL;
        return OSKIT_E_NOINTERFACE;
}

static OSKIT_COMDECL
ttystream_query(oskit_ttystream_t *f, const struct oskit_guid *iid, 
	void **out_ihandle)
{
        oskit_ttyimpl_t *b = (oskit_ttyimpl_t *)f;
	return query(b, iid, out_ihandle);
}

static OSKIT_COMDECL
asyncio_query(oskit_asyncio_t *f, const struct oskit_guid *iid,
	void **out_ihandle)
{
        oskit_ttyimpl_t *b = (oskit_ttyimpl_t *)(f-1);
	return query(b, iid, out_ihandle);
}

/*
 * addref
 */
static OSKIT_COMDECL_U
addref(oskit_ttyimpl_t *b)
{
	unsigned	newcount;
	b->before(b->cookie);
	newcount = ++b->count;
	b->after(b->cookie);
	return newcount;
}

static OSKIT_COMDECL_U
ttystream_addref(oskit_ttystream_t *f)
{
        oskit_ttyimpl_t *b = (oskit_ttyimpl_t *)f;
	return addref(b);
}

static OSKIT_COMDECL_U
asyncio_addref(oskit_asyncio_t *f)
{
        oskit_ttyimpl_t *b = (oskit_ttyimpl_t *)(f-1);
	return addref(b);
}

/*
 * release. The contained ttyimpl was released, so release the wrapper.
 */
static OSKIT_COMDECL_U
release(oskit_ttyimpl_t *b)
{
        unsigned newcount;

	b->before(b->cookie);
	if ((newcount = --b->count) == 0) {
		if (b->w_ios)
			oskit_ttystream_release(b->w_ios);
		if (b->w_ioa)
			oskit_asyncio_release(b->w_ioa);
		b->after(b->cookie);
		free(b);
	} else {
		b->after(b->cookie);
	}
	return newcount;
}

static OSKIT_COMDECL_U
ttystream_release(oskit_ttystream_t *f)
{
        oskit_ttyimpl_t *b = (oskit_ttyimpl_t *)f;
	return release(b);
}

static OSKIT_COMDECL_U
asyncio_release(oskit_asyncio_t *f)
{
        oskit_ttyimpl_t *b = (oskit_ttyimpl_t *)(f-1);
	return release(b);
}

/*******************************************************/
/******* Implementation of the oskit_ttystream_t if ********/
/*******************************************************/

static OSKIT_COMDECL
ttystream_read(oskit_ttystream_t *f,
	       void *buf, oskit_u32_t len, oskit_u32_t *out_actual)
{
	oskit_ttyimpl_t *s = (oskit_ttyimpl_t *)(f);
	oskit_error_t	 err;

	err = WRAPPERCALL(s, ios, ttystream, read, buf, len, out_actual);

	return err;
}

static OSKIT_COMDECL
ttystream_write(oskit_ttystream_t *f, const void *buf,
		oskit_u32_t len, oskit_u32_t *out_actual)
{
	oskit_ttyimpl_t *s = (oskit_ttyimpl_t *)(f);
	oskit_error_t	 err;

	err = WRAPPERCALL(s, ios, ttystream, write, buf, len, out_actual);

	return err;
}

static OSKIT_COMDECL
ttystream_seek(oskit_ttystream_t *f, oskit_s64_t ofs,
	       oskit_seek_t whence, oskit_u64_t *out_newpos)
{
	oskit_ttyimpl_t *s = (oskit_ttyimpl_t *)(f);
	oskit_error_t	 err;

	err = WRAPPERCALL(s, ios, ttystream, seek, ofs, whence, out_newpos);

	return err;
}

static OSKIT_COMDECL
ttystream_setsize(oskit_ttystream_t *f, oskit_u64_t new_size)
{
	oskit_ttyimpl_t *s = (oskit_ttyimpl_t *)(f);
	oskit_error_t	 err;

	err = WRAPPERCALL(s, ios, ttystream, setsize, new_size);

	return err;
}

static OSKIT_COMDECL
ttystream_copyto(oskit_ttystream_t *f, oskit_ttystream_t *dst,
		 oskit_u64_t size,
		 oskit_u64_t *out_read,
		 oskit_u64_t *out_written)
{
	oskit_ttyimpl_t *s = (oskit_ttyimpl_t *)(f);
	oskit_error_t	 err;

	err = WRAPPERCALL(s, ios, ttystream, copyto,
			  dst, size, out_read, out_written);

	return err;
}

static OSKIT_COMDECL
ttystream_commit(oskit_ttystream_t *f, oskit_u32_t commit_flags)
{
	oskit_ttyimpl_t *s = (oskit_ttyimpl_t *)(f);
	oskit_error_t	 err;

	err = WRAPPERCALL(s, ios, ttystream, commit, commit_flags);

	return err;
}

static OSKIT_COMDECL
ttystream_revert(oskit_ttystream_t *f)
{
	oskit_ttyimpl_t *s = (oskit_ttyimpl_t *)(f);
	oskit_error_t	 err;

	err = WRAPPERCALL_NOARGS(s, ios, ttystream, revert);

	return err;
}

static OSKIT_COMDECL
ttystream_lock_region(oskit_ttystream_t *f,
		      oskit_u64_t offset,
		      oskit_u64_t size, oskit_u32_t lock_type)
{
	oskit_ttyimpl_t *s = (oskit_ttyimpl_t *)(f);
	oskit_error_t	 err;

	err = WRAPPERCALL(s, ios, ttystream, lockregion,
			  offset, size, lock_type);

	return err;
}

static OSKIT_COMDECL
ttystream_unlock_region(oskit_ttystream_t *f,
			oskit_u64_t offset,
			oskit_u64_t size, oskit_u32_t lock_type)
{
	oskit_ttyimpl_t *s = (oskit_ttyimpl_t *)(f);
	oskit_error_t	 err;

	err = WRAPPERCALL(s, ios, ttystream, unlockregion,
			  offset, size, lock_type);

	return err;
}

static OSKIT_COMDECL
ttystream_stat(oskit_ttystream_t *f, oskit_stream_stat_t *out_stat,
	       oskit_u32_t stat_flags)
{
	oskit_ttyimpl_t *s = (oskit_ttyimpl_t *)(f);
	oskit_error_t	 err;

	err = WRAPPERCALL(s, ios, ttystream, stat, out_stat, stat_flags);

	return err;
}

static OSKIT_COMDECL
ttystream_clone(oskit_ttystream_t *f, oskit_ttystream_t **out_stream)
{
	oskit_ttyimpl_t		*s = (oskit_ttyimpl_t *)(f);
	oskit_error_t		err;
	oskit_ttystream_t	*str;

	err = WRAPPERCALL(s, ios, ttystream, clone, &str);
	if (err == 0) {
		/* wrap the stream interface only !? */
		err = oskit_wrap_ttystream(str, 
					   s->before, s->after, s->cookie,
					   out_stream);
		oskit_ttystream_release(str);
	}
	return err;
}

static OSKIT_COMDECL
ttystream_getattr(oskit_ttystream_t *f, struct oskit_termios *out_attr)
{
	oskit_ttyimpl_t *s = (oskit_ttyimpl_t *)(f);
	oskit_error_t	 err;

	err = WRAPPERCALL(s, ios, ttystream, getattr, out_attr);

	return err;
}

static OSKIT_COMDECL
ttystream_setattr(oskit_ttystream_t *f,
		  int actions, const struct oskit_termios *attr)
{
	oskit_ttyimpl_t *s = (oskit_ttyimpl_t *)(f);
	oskit_error_t	 err;

	err = WRAPPERCALL(s, ios, ttystream, setattr, actions, attr);

	return err;
}

static OSKIT_COMDECL
ttystream_sendbreak(oskit_ttystream_t *f, oskit_u32_t duration)
{
	oskit_ttyimpl_t *s = (oskit_ttyimpl_t *)(f);
	oskit_error_t	 err;

	err = WRAPPERCALL(s, ios, ttystream, sendbreak, duration);

	return err;
}

static OSKIT_COMDECL
ttystream_drain(oskit_ttystream_t *f)
{
	oskit_ttyimpl_t *s = (oskit_ttyimpl_t *)(f);
	oskit_error_t	 err;

	err = WRAPPERCALL_NOARGS(s, ios, ttystream, drain);

	return err;
}

static OSKIT_COMDECL
ttystream_flush(oskit_ttystream_t *f, int queue_selector)
{
	oskit_ttyimpl_t *s = (oskit_ttyimpl_t *)(f);
	oskit_error_t	 err;

	err = WRAPPERCALL(s, ios, ttystream, flush, queue_selector);

	return err;
}

static OSKIT_COMDECL
ttystream_flow(oskit_ttystream_t *f, int action)
{
	oskit_ttyimpl_t *s = (oskit_ttyimpl_t *)(f);
	oskit_error_t	 err;

	err = WRAPPERCALL(s, ios, ttystream, flow, action);

	return err;
}

static OSKIT_COMDECL
ttystream_getpgrp(oskit_ttystream_t *f, oskit_pid_t *out_pid)
{
	oskit_ttyimpl_t *s = (oskit_ttyimpl_t *)(f);
	oskit_error_t	 err;

	err = WRAPPERCALL(s, ios, ttystream, getpgrp, out_pid);

	return err;
}

static OSKIT_COMDECL
ttystream_setpgrp(oskit_ttystream_t *f, oskit_pid_t new_pid)
{
	oskit_ttyimpl_t *s = (oskit_ttyimpl_t *)(f);
	oskit_error_t	 err;

	err = WRAPPERCALL(s, ios, ttystream, setpgrp, new_pid);

	return err;
}

static OSKIT_COMDECL
ttystream_ttyname(oskit_ttystream_t *f, char **out_name)
{
	oskit_ttyimpl_t *s = (oskit_ttyimpl_t *)(f);
	oskit_error_t	 err;

	err = WRAPPERCALL(s, ios, ttystream, ttyname, out_name);

	return err;
}

static OSKIT_COMDECL
ttystream_getsid(oskit_ttystream_t *f, oskit_pid_t *out_pid)
{
	oskit_ttyimpl_t *s = (oskit_ttyimpl_t *)(f);
	oskit_error_t	 err;

	err = WRAPPERCALL(s, ios, ttystream, getsid, out_pid);

	return err;
}

/*******************************************************/
/******* Implementation of the oskit_asyncio_t if *******/
/*******************************************************/

static OSKIT_COMDECL
asyncio_poll(oskit_asyncio_t *f)
{
	oskit_ttyimpl_t *si = (oskit_ttyimpl_t *)(f-1);
	oskit_error_t	 err;

	err = WRAPPERCALL_NOARGS(si, ioa, asyncio, poll);

	return err;
}

static OSKIT_COMDECL
asyncio_add_listener(oskit_asyncio_t *f, struct oskit_listener *l,
	oskit_s32_t mask)
{
	oskit_ttyimpl_t *si = (oskit_ttyimpl_t *)(f-1);
	oskit_error_t	 err;

	err = WRAPPERCALL(si, ioa, asyncio, add_listener, l, mask);

	return err;
}

static OSKIT_COMDECL
asyncio_remove_listener(oskit_asyncio_t *f, struct oskit_listener *l0)
{
	oskit_ttyimpl_t *si = (oskit_ttyimpl_t *)(f-1);
	oskit_error_t	 err;

	err = WRAPPERCALL(si, ioa, asyncio, remove_listener, l0);

	return err;
}
/*
 * return the number of bytes that can be read, basically ioctl(FIONREAD)
 */
static OSKIT_COMDECL
asyncio_readable(oskit_asyncio_t *f)
{
	oskit_ttyimpl_t *si = (oskit_ttyimpl_t *)(f-1);
	oskit_error_t	 err;

	err = WRAPPERCALL_NOARGS(si, ioa, asyncio, readable);

	return err;
}

/***************************************************************************/
/*
 * vtables for interfaces exported by ttytreams
 */
static struct oskit_ttystream_ops ttystreamops =
{
	ttystream_query,
	ttystream_addref,
	ttystream_release,
	ttystream_read,
	ttystream_write,
	ttystream_seek,
	ttystream_setsize,
	ttystream_copyto,
	ttystream_commit,
	ttystream_revert,
	ttystream_lock_region,
	ttystream_unlock_region,
	ttystream_stat,
	ttystream_clone,
	ttystream_getattr, 
	ttystream_setattr, 
	ttystream_sendbreak, 
	ttystream_drain, 
	ttystream_flush, 
	ttystream_flow, 
	ttystream_getpgrp, 
	ttystream_setpgrp,
	ttystream_ttyname, 
	ttystream_getsid
};

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
 * Wrap a ttyimpl in the thread specific version.
 */
oskit_error_t
oskit_wrap_ttystream(struct oskit_ttystream *in,
        void (*before)(),
	void (*after)(),
	void *cookie,
	struct oskit_ttystream **out)
{
        oskit_ttyimpl_t *newtty = malloc(sizeof(*newtty));

        if (newtty == NULL)
                return OSKIT_ENOMEM;
        memset(newtty, 0, sizeof(*newtty));

        newtty->count = 1;
        newtty->ios.ops = &ttystreamops;
        newtty->ioa.ops = &asyncioops;
	newtty->w_ios = in;
	oskit_ttystream_addref(newtty->w_ios);

	newtty->before = before;
	newtty->after = after;
	newtty->cookie = cookie;

	*out = &newtty->ios;
	return 0;
}
