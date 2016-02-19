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
 * Definitions for a COM object containing and wrapping the four interfaces
 * contained in our socket implementation - socket, stream, asyncio, and
 * bufio_stream.
 */

#include <oskit/com.h>
#include <oskit/com/wrapper.h>
#include <oskit/net/socket.h>
#include <oskit/c/string.h>
#include <oskit/c/stdlib.h>
#include <oskit/c/stdio.h>
#include <oskit/io/asyncio.h>
#include <oskit/io/bufio_stream.h>

/*
 * Wrapped up socket interfaces.
 */
typedef struct {
	oskit_socket_t		ioi;		/* COM I/O interface Nr. 1 */
	oskit_stream_t		ios;		/* COM I/O interface Nr. 2 */
	oskit_asyncio_t		ioa;		/* COM I/O interface Nr. 3 */
	oskit_bufio_stream_t	iob;		/* COM I/O interface Nr. 4 */
	unsigned		count;		/* reference count */
	oskit_socket_t		*w_ioi;		/* COM I/O interface Nr. 1 */
	oskit_stream_t		*w_ios;		/* COM I/O interface Nr. 2 */
	oskit_asyncio_t		*w_ioa;		/* COM I/O interface Nr. 3 */
	oskit_bufio_stream_t	*w_iob;		/* COM I/O interface Nr. 4 */
	void			(*before)(void *);
	void			(*after)(void *);
	void			*cookie;
} oskit_sockimpl_t;

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

static OSKIT_COMDECL_U addref(oskit_sockimpl_t *b);
/*
 * query
 */

static OSKIT_COMDECL
query(oskit_sockimpl_t *b, const struct oskit_guid *iid, void **out_ihandle)
{
	oskit_error_t rc = 0;

	b->before(b->cookie);
        if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
	    memcmp(iid, &oskit_posixio_iid, sizeof(*iid)) == 0 ||
            memcmp(iid, &oskit_socket_iid, sizeof(*iid)) == 0) {
		*out_ihandle = &b->ioi;
		++b->count;
		b->after(b->cookie);
		return 0;
        }

        if (memcmp(iid, &oskit_stream_iid, sizeof(*iid)) == 0) {
		if (b->w_ios == 0) 
			rc = oskit_socket_query(b->w_ioi, 
				&oskit_stream_iid, (void **)&b->w_ios);
		if (rc == 0) {
			*out_ihandle = &b->ios;
			++b->count;
			b->after(b->cookie);
			return 0;
		}
        }

        if (memcmp(iid, &oskit_bufio_stream_iid, sizeof(*iid)) == 0) {
		if (b->w_iob == 0) 
			rc = oskit_socket_query(b->w_ioi, 
				&oskit_bufio_stream_iid, (void **)&b->w_iob);
		if (rc == 0) {
			*out_ihandle = &b->iob;
			++b->count;
			b->after(b->cookie);
			return 0;
		}
	}

        if (memcmp(iid, &oskit_asyncio_iid, sizeof(*iid)) == 0) {
		if (b->w_ioa == 0) 
			rc = oskit_socket_query(b->w_ioi, 
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
socket_query(oskit_socket_t *f, const struct oskit_guid *iid, 
	void **out_ihandle)
{
        oskit_sockimpl_t *b = (oskit_sockimpl_t *)f;
	return query(b, iid, out_ihandle);
}

static OSKIT_COMDECL
opensocket_query(oskit_stream_t *f, const struct oskit_guid *iid,
				 void **out_ihandle)
{
        oskit_sockimpl_t *b = (oskit_sockimpl_t *)(f-1);
	return query(b, iid, out_ihandle);
}

static OSKIT_COMDECL
asyncio_query(oskit_asyncio_t *f, const struct oskit_guid *iid,
	void **out_ihandle)
{
        oskit_sockimpl_t *b = (oskit_sockimpl_t *)(f-2);
	return query(b, iid, out_ihandle);
}

static OSKIT_COMDECL
bufio_stream_query(oskit_bufio_stream_t *f, const struct oskit_guid *iid,
	void **out_ihandle)
{
        oskit_sockimpl_t *b = (oskit_sockimpl_t *)(f-3);
	return query(b, iid, out_ihandle);
}

/*
 * addref
 */
static OSKIT_COMDECL_U
addref(oskit_sockimpl_t *b)
{
	unsigned	newcount;
	b->before(b->cookie);
	newcount = ++b->count;
	b->after(b->cookie);
	return newcount;
}

static OSKIT_COMDECL_U
socket_addref(oskit_socket_t *f)
{
        oskit_sockimpl_t *b = (oskit_sockimpl_t *)f;
	return addref(b);
}

static OSKIT_COMDECL_U
opensocket_addref(oskit_stream_t *f)
{
        oskit_sockimpl_t *b = (oskit_sockimpl_t *)(f-1);
	return addref(b);
}

static OSKIT_COMDECL_U
asyncio_addref(oskit_asyncio_t *f)
{
        oskit_sockimpl_t *b = (oskit_sockimpl_t *)(f-2);
	return addref(b);
}

static OSKIT_COMDECL_U
bufio_stream_addref(oskit_bufio_stream_t *f)
{
        oskit_sockimpl_t *b = (oskit_sockimpl_t *)(f-3);
	return addref(b);
}

/*
 * release. The contained sockimpl was released, so release the wrapper.
 */
static OSKIT_COMDECL_U
release(oskit_sockimpl_t *b)
{
        unsigned newcount;

	b->before(b->cookie);
	if ((newcount = --b->count) == 0) {
		if (b->w_ioi)
			oskit_socket_release(b->w_ioi);
		if (b->w_ios)
			oskit_stream_release(b->w_ios);
		if (b->w_ioa)
			oskit_asyncio_release(b->w_ioa);
		if (b->w_iob)
			oskit_bufio_stream_release(b->w_iob);
		b->after(b->cookie);
		free(b);
	} else {
		b->after(b->cookie);
	}
	return newcount;
}

static OSKIT_COMDECL_U
socket_release(oskit_socket_t *f)
{
        oskit_sockimpl_t *b = (oskit_sockimpl_t *)f;
	return release(b);
}

static OSKIT_COMDECL_U
opensocket_release(oskit_stream_t *f)
{
        oskit_sockimpl_t *b = (oskit_sockimpl_t *)(f-1);
	return release(b);
}

static OSKIT_COMDECL_U
asyncio_release(oskit_asyncio_t *f)
{
        oskit_sockimpl_t *b = (oskit_sockimpl_t *)(f-2);
	return release(b);
}

static OSKIT_COMDECL_U
bufio_stream_release(oskit_bufio_stream_t *f)
{
        oskit_sockimpl_t *b = (oskit_sockimpl_t *)(f-3);
	return release(b);
}

/*******************************************************/
/******* Implementation of the oskit_socket_t if ********/
/*******************************************************/

/*** Operations inherited from oskit_posixio_t ***/
static OSKIT_COMDECL
socket_stat(oskit_socket_t *f, struct oskit_stat *out_stats)
{
        oskit_sockimpl_t *b = (oskit_sockimpl_t *)(f);
	oskit_error_t	 err;

	err = WRAPPERCALL(b, ioi, socket, stat, out_stats);

	return err;
}

static OSKIT_COMDECL
socket_setstat(oskit_socket_t *f, oskit_u32_t mask,
	const struct oskit_stat *stats)
{
        oskit_sockimpl_t *b = (oskit_sockimpl_t *)(f);
	oskit_error_t	 err;

	err = WRAPPERCALL(b, ioi, socket, setstat, mask, stats);

	return err;
}

static OSKIT_COMDECL
socket_pathconf(oskit_socket_t *f, oskit_s32_t option, oskit_s32_t *out_val)
{
        oskit_sockimpl_t *b = (oskit_sockimpl_t *)(f);
	oskit_error_t	 err;

	err = WRAPPERCALL(b, ioi, socket, pathconf, option, out_val);

	return err;
}

/*** Operations specific to oskit_socket_t ***/
static OSKIT_COMDECL
socket_accept(oskit_socket_t *s, struct oskit_sockaddr *name,
	oskit_size_t *anamelen, struct oskit_socket **newopenso)
{
        oskit_sockimpl_t *b = (oskit_sockimpl_t *)(s);
	oskit_error_t	 err;
	oskit_socket_t	 *newso;

	err = WRAPPERCALL(b, ioi, socket, accept, name, anamelen, &newso);
	if (err == 0) {
		err = oskit_wrap_sockio(newso,
			b->before, b->after, b->cookie, newopenso);
		oskit_socket_release(newso);
	}
	return err;
}

static OSKIT_COMDECL
socket_bind(oskit_socket_t *s, const struct oskit_sockaddr *name,
	oskit_size_t namelen)
{
        oskit_sockimpl_t *b = (oskit_sockimpl_t *)(s);
	oskit_error_t	 err;

	err = WRAPPERCALL(b, ioi, socket, bind, name, namelen);

	return err;
}

static OSKIT_COMDECL
socket_connect(oskit_socket_t *s, const struct oskit_sockaddr *name,
	oskit_size_t namelen)
{
        oskit_sockimpl_t *b = (oskit_sockimpl_t *)(s);
	oskit_error_t	 err;

	err = WRAPPERCALL(b, ioi, socket, connect, name, namelen);

	return err;
}

static OSKIT_COMDECL
socket_listen(oskit_socket_t *s, oskit_u32_t n)
{
        oskit_sockimpl_t *b = (oskit_sockimpl_t *)(s);
	oskit_error_t	 err;

	err = WRAPPERCALL(b, ioi, socket, listen, n);

	return err;
}

static OSKIT_COMDECL
socket_getsockname(oskit_socket_t *s, struct oskit_sockaddr *asa,
	oskit_size_t *alen)
{
        oskit_sockimpl_t *b = (oskit_sockimpl_t *)(s);
	oskit_error_t	 err;

	err = WRAPPERCALL(b, ioi, socket, getsockname, asa, alen);

	return err;
}

static OSKIT_COMDECL
socket_setsockopt(oskit_socket_t *s, oskit_u32_t level, oskit_u32_t name,
		const void *val, oskit_size_t valsize)
{
        oskit_sockimpl_t *b = (oskit_sockimpl_t *)(s);
	oskit_error_t	 err;

	err = WRAPPERCALL(b, ioi, socket, setsockopt,
			  level, name, val, valsize);

	return err;
}

static OSKIT_COMDECL
socket_sendto(oskit_socket_t *s, const void *msg, oskit_size_t len,
		oskit_u32_t flags, const struct oskit_sockaddr *to,
		oskit_size_t tolen, oskit_size_t *retval)
{
        oskit_sockimpl_t *b = (oskit_sockimpl_t *)(s);
	oskit_error_t	 err;

	err = WRAPPERCALL(b, ioi, socket, sendto,
			  msg, len, flags, to, tolen, retval);

	return err;
}

static OSKIT_COMDECL
socket_recvfrom(oskit_socket_t *s, void *buf,
		oskit_size_t len, oskit_u32_t flags,
		struct oskit_sockaddr *from, oskit_size_t *fromlen,
		oskit_size_t *retval)
{
        oskit_sockimpl_t *b = (oskit_sockimpl_t *)(s);
	oskit_error_t	 err;

	err = WRAPPERCALL(b, ioi, socket, recvfrom,
			  buf, len, flags, from, fromlen, retval);

	return err;
}

static OSKIT_COMDECL
socket_getsockopt(oskit_socket_t *s, oskit_u32_t level, oskit_u32_t name,
		void *val, oskit_size_t *valsize)
{
        oskit_sockimpl_t *b = (oskit_sockimpl_t *)(s);
	oskit_error_t	 err;

	err = WRAPPERCALL(b, ioi, socket, getsockopt,
			  level, name, val, valsize);

	return err;
}

static OSKIT_COMDECL
socket_sendmsg(oskit_socket_t *s, const struct oskit_msghdr *msg,
		oskit_u32_t flags, oskit_size_t *retval)
{
        oskit_sockimpl_t *b = (oskit_sockimpl_t *)(s);
	oskit_error_t	 err;

	err = WRAPPERCALL(b, ioi, socket, sendmsg, msg, flags, retval);

	return err;
}

static OSKIT_COMDECL
socket_recvmsg(oskit_socket_t *s, struct oskit_msghdr *msg,
		oskit_u32_t flags, oskit_size_t *retval)
{
        oskit_sockimpl_t *b = (oskit_sockimpl_t *)(s);
	oskit_error_t	 err;

	err = WRAPPERCALL(b, ioi, socket, recvmsg, msg, flags, retval);

	return err;
}

static OSKIT_COMDECL
socket_getpeername(oskit_socket_t *f,
	struct oskit_sockaddr *asa, oskit_size_t *alen)
{
        oskit_sockimpl_t *b = (oskit_sockimpl_t *)(f);
	oskit_error_t	 err;

	err = WRAPPERCALL(b, ioi, socket, getpeername, asa, alen);

	return err;
}

static OSKIT_COMDECL
socket_shutdown(oskit_socket_t *f, oskit_u32_t how)
{
        oskit_sockimpl_t *b = (oskit_sockimpl_t *)(f);
	oskit_error_t	 err;

	err = WRAPPERCALL(b, ioi, socket, shutdown, how);

	return err;
}

/*******************************************************/
/******* Implementation of the oskit_stream_t if ********/
/*******************************************************/

/*** Operations inherited from oskit_stream interface ***/
static OSKIT_COMDECL
opensocket_read(oskit_stream_t *f, void *buf, oskit_u32_t len,
				oskit_u32_t *out_actual)
{
	oskit_sockimpl_t *s = (oskit_sockimpl_t *)(f-1);
	oskit_error_t	 err;

	err = WRAPPERCALL(s, ios, stream, read, buf, len, out_actual);

	return err;
}

static OSKIT_COMDECL
opensocket_write(oskit_stream_t *f, const void *buf,
				 oskit_u32_t len, oskit_u32_t *out_actual)
{
	oskit_sockimpl_t *s = (oskit_sockimpl_t *)(f-1);
	oskit_error_t	 err;

	err = WRAPPERCALL(s, ios, stream, write, buf, len, out_actual);

	return err;
}

static OSKIT_COMDECL
opensocket_seek(oskit_stream_t *f, oskit_s64_t ofs,
	oskit_seek_t whence, oskit_u64_t *out_newpos)
{
	oskit_sockimpl_t *s = (oskit_sockimpl_t *)(f-1);
	oskit_error_t	 err;

	err = WRAPPERCALL(s, ios, stream, seek, ofs, whence, out_newpos);

	return err;
}

static OSKIT_COMDECL
opensocket_setsize(oskit_stream_t *f, oskit_u64_t new_size)
{
	oskit_sockimpl_t *s = (oskit_sockimpl_t *)(f-1);
	oskit_error_t	 err;

	err = WRAPPERCALL(s, ios, stream, setsize, new_size);

	return err;
}

static OSKIT_COMDECL
opensocket_copyto(oskit_stream_t *f, oskit_stream_t *dst,
	oskit_u64_t size,
	oskit_u64_t *out_read,
	oskit_u64_t *out_written)
{
	oskit_sockimpl_t *s = (oskit_sockimpl_t *)(f-1);
	oskit_error_t	 err;

	err = WRAPPERCALL(s, ios, stream, copyto,
			  dst, size, out_read, out_written);

	return err;
}

static OSKIT_COMDECL
opensocket_commit(oskit_stream_t *f, oskit_u32_t commit_flags)
{
	oskit_sockimpl_t *s = (oskit_sockimpl_t *)(f-1);
	oskit_error_t	 err;

	err = WRAPPERCALL(s, ios, stream, commit, commit_flags);

	return err;
}

static OSKIT_COMDECL
opensocket_revert(oskit_stream_t *f)
{
	oskit_sockimpl_t *s = (oskit_sockimpl_t *)(f-1);
	oskit_error_t	 err;

	err = WRAPPERCALL_NOARGS(s, ios, stream, revert);

	return err;
}

static OSKIT_COMDECL
opensocket_lock_region(oskit_stream_t *f,
	oskit_u64_t offset, oskit_u64_t size, oskit_u32_t lock_type)
{
	oskit_sockimpl_t *s = (oskit_sockimpl_t *)(f-1);
	oskit_error_t	 err;

	err = WRAPPERCALL(s, ios, stream, lockregion,
			  offset, size, lock_type);

	return err;
}

static OSKIT_COMDECL
opensocket_unlock_region(oskit_stream_t *f,
	oskit_u64_t offset, oskit_u64_t size, oskit_u32_t lock_type)
{
	oskit_sockimpl_t *s = (oskit_sockimpl_t *)(f-1);
	oskit_error_t	 err;

	err = WRAPPERCALL(s, ios, stream, unlockregion,
			  offset, size, lock_type);

	return err;
}

static OSKIT_COMDECL
opensocket_stat(oskit_stream_t *f, oskit_stream_stat_t *out_stat,
	oskit_u32_t stat_flags)
{
	oskit_sockimpl_t *s = (oskit_sockimpl_t *)(f-1);
	oskit_error_t	 err;

	err = WRAPPERCALL(s, ios, stream, stat, out_stat, stat_flags);

	return err;
}

static OSKIT_COMDECL
opensocket_clone(oskit_stream_t *f, oskit_stream_t **out_stream)
{
	oskit_sockimpl_t *s = (oskit_sockimpl_t *)(f-1);
	oskit_error_t	 err;
	oskit_stream_t	*str;

	err = WRAPPERCALL(s, ios, stream, clone, &str);
	if (err == 0) {
		/* wrap the stream interface only !? */
		err = oskit_wrap_stream(str, 
			s->before, s->after, s->cookie,
			out_stream);
		oskit_stream_release(str);
	}
	return err;
}

/*******************************************************/
/******* Implementation of the oskit_asyncio_t if *******/
/*******************************************************/

static OSKIT_COMDECL
asyncio_poll(oskit_asyncio_t *f)
{
	oskit_sockimpl_t *si = (oskit_sockimpl_t *)(f-2);
	oskit_error_t	 err;

	err = WRAPPERCALL_NOARGS(si, ioa, asyncio, poll);

	return err;
}

static OSKIT_COMDECL
asyncio_add_listener(oskit_asyncio_t *f, struct oskit_listener *l,
	oskit_s32_t mask)
{
	oskit_sockimpl_t *si = (oskit_sockimpl_t *)(f-2);
	oskit_error_t	 err;

	err = WRAPPERCALL(si, ioa, asyncio, add_listener, l, mask);

	return err;
}

static OSKIT_COMDECL
asyncio_remove_listener(oskit_asyncio_t *f, struct oskit_listener *l0)
{
	oskit_sockimpl_t *si = (oskit_sockimpl_t *)(f-2);
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
	oskit_sockimpl_t *si = (oskit_sockimpl_t *)(f-2);
	oskit_error_t	 err;

	err = WRAPPERCALL_NOARGS(si, ioa, asyncio, readable);

	return err;
}

/***************************************************************************/
/*
 * methods of bufio_streams
 */
static OSKIT_COMDECL
bufio_stream_read(oskit_bufio_stream_t *f,
	struct oskit_bufio **buf, oskit_size_t *bytes)
{
	oskit_sockimpl_t *si = (oskit_sockimpl_t *)(f-3);
	oskit_error_t	 err;

	err = WRAPPERCALL(si, iob, bufio_stream, read, buf, bytes);

	return err;
}

static OSKIT_COMDECL
bufio_stream_write(oskit_bufio_stream_t *f,
	struct oskit_bufio *buf, oskit_size_t offset)
{
	oskit_sockimpl_t *si = (oskit_sockimpl_t *)(f-3);
	oskit_error_t	 err;

	err = WRAPPERCALL(si, iob, bufio_stream, write, buf, offset);

	return err;
}

/***************************************************************************/
/*
 * vtables for interfaces exported by sockets
 */
static struct oskit_socket_ops sockops =
{
	socket_query,
	socket_addref,
	socket_release,
	socket_stat,
	socket_setstat,
	socket_pathconf,
	socket_accept,
	socket_bind,
	socket_connect,
	socket_shutdown,
	socket_listen,
	socket_getsockname,
	socket_getpeername,
	socket_getsockopt,
	socket_setsockopt,
	socket_sendto,
	socket_recvfrom,
	socket_sendmsg,
	socket_recvmsg
};

static struct oskit_stream_ops opensockops =
{
	opensocket_query,
	opensocket_addref,
	opensocket_release,
	opensocket_read,
	opensocket_write,
	opensocket_seek,
	opensocket_setsize,
	opensocket_copyto,
	opensocket_commit,
	opensocket_revert,
	opensocket_lock_region,
	opensocket_unlock_region,
	opensocket_stat,
	opensocket_clone
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

static struct oskit_bufio_stream_ops bufio_streamops =
{
	bufio_stream_query,
	bufio_stream_addref,
	bufio_stream_release,
	bufio_stream_read,
	bufio_stream_write
};

/*
 * Wrap a sockimpl in the thread specific version.
 */
oskit_error_t
oskit_wrap_sockio(struct oskit_socket *in,
        void (*before)(),
	void (*after)(),
	void *cookie,
	struct oskit_socket **out)
{
        oskit_sockimpl_t *newsi = malloc(sizeof(*newsi));

        if (newsi == NULL)
                return OSKIT_ENOMEM;
        memset(newsi, 0, sizeof(*newsi));

        newsi->count = 1;
        newsi->ioi.ops = &sockops;
        newsi->ios.ops = &opensockops;
        newsi->ioa.ops = &asyncioops;
        newsi->iob.ops = &bufio_streamops;
	newsi->w_ioi = in;
	oskit_socket_addref(newsi->w_ioi);

	newsi->before = before;
	newsi->after = after;
	newsi->cookie = cookie;

	*out = &newsi->ioi;
	return 0;
}
