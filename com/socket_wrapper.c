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
 * Definitions of a simple wrapper that can be used to protect oskit_socket
 * interfaces.
 */

#include <oskit/net/socket.h>
#include <oskit/com/wrapper.h>
#include <oskit/c/stdlib.h>
#include <oskit/c/string.h>

typedef struct {
	oskit_socket_t	ioi;		/* COM interface being exported */
	int		count;		/* reference count */
	oskit_socket_t	*wrapped;	/* COM interface being wrapped */
	void		(*before)(void *);
	void		(*after)(void *);
	void		*cookie;
} socketwrap_impl_t;

#define WRAPPERCALL(impl, call, args...) ({                      	\
        oskit_error_t __err;                                            \
        impl->before(impl->cookie);                                     \
        __err = oskit_socket_##call(impl->wrapped ,##args);             \
        impl->after(impl->cookie);                                      \
        __err; })

#define WRAPPERCALL_NOARGS(impl, call) ({                        	\
        oskit_error_t __err;                                            \
        impl->before(impl->cookie);                                     \
        __err = oskit_socket_##call(impl->wrapped);                     \
        impl->after(impl->cookie);                              	\
        __err; })


static OSKIT_COMDECL
socket_query(oskit_socket_t *f, const struct oskit_guid *iid,
	void **out_ihandle)
{
        socketwrap_impl_t  *b = (socketwrap_impl_t *)f;

	if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
            memcmp(iid, &oskit_socket_iid, sizeof(*iid)) == 0) {
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
socket_addref(oskit_socket_t *f)
{
        socketwrap_impl_t *b = (socketwrap_impl_t *)f;
	unsigned newcount;

	b->before(b->cookie);
	newcount = ++b->count;
	b->after(b->cookie);
        return newcount;
}

static OSKIT_COMDECL_U
socket_release(oskit_socket_t *f)
{
        socketwrap_impl_t *b = (socketwrap_impl_t *)f;
        unsigned newcount;

	b->before(b->cookie);
        if ((newcount = --b->count) == 0) {
		oskit_socket_release(b->wrapped);
		b->after(b->cookie);
		free(b);
	} else
		b->after(b->cookie);
	return newcount;
}

/*** Operations inherited from oskit_posixio_t ***/
static OSKIT_COMDECL
socket_stat(oskit_socket_t *f, struct oskit_stat *out_stats)
{
        socketwrap_impl_t *b = (socketwrap_impl_t *)(f);
        oskit_error_t    err;

        err = WRAPPERCALL(b, stat, out_stats);

        return err;
}

static OSKIT_COMDECL
socket_setstat(oskit_socket_t *f, oskit_u32_t mask,
        const struct oskit_stat *stats)
{
        socketwrap_impl_t *b = (socketwrap_impl_t *)(f);
        oskit_error_t    err;

        err = WRAPPERCALL(b, setstat, mask, stats);

        return err;
}

static OSKIT_COMDECL
socket_pathconf(oskit_socket_t *f, oskit_s32_t option, oskit_s32_t *out_val)
{
        socketwrap_impl_t *b = (socketwrap_impl_t *)(f);
        oskit_error_t    err;

        err = WRAPPERCALL(b, pathconf, option, out_val);

        return err;
}

/*** Operations specific to oskit_socket_t ***/
static OSKIT_COMDECL
socket_accept(oskit_socket_t *s, struct oskit_sockaddr *name,
        oskit_size_t *anamelen, struct oskit_socket **newopenso)
{
        socketwrap_impl_t *b = (socketwrap_impl_t *)(s);
	oskit_socket_t	 *newso;
        oskit_error_t    err;

        err = WRAPPERCALL(b, accept, name, anamelen, &newso);

	if (err == 0) {
		err = oskit_wrap_socket(newso,
			b->before, b->after, b->cookie, newopenso);
		oskit_socket_release(newso);
	}

        return err;
}

static OSKIT_COMDECL
socket_bind(oskit_socket_t *s, const struct oskit_sockaddr *name,
        oskit_size_t namelen)
{
        socketwrap_impl_t *b = (socketwrap_impl_t *)(s);
        oskit_error_t    err;

        err = WRAPPERCALL(b, bind, name, namelen);

        return err;
}

static OSKIT_COMDECL
socket_connect(oskit_socket_t *s, const struct oskit_sockaddr *name,
        oskit_size_t namelen)
{
        socketwrap_impl_t *b = (socketwrap_impl_t *)(s);
        oskit_error_t    err;

        err = WRAPPERCALL(b, connect, name, namelen);

        return err;
}

static OSKIT_COMDECL
socket_listen(oskit_socket_t *s, oskit_u32_t n)
{
        socketwrap_impl_t *b = (socketwrap_impl_t *)(s);
        oskit_error_t    err;

        err = WRAPPERCALL(b, listen, n);

        return err;
}

static OSKIT_COMDECL
socket_getsockname(oskit_socket_t *s, struct oskit_sockaddr *asa,
        oskit_size_t *alen)
{
        socketwrap_impl_t *b = (socketwrap_impl_t *)(s);
        oskit_error_t    err;

        err = WRAPPERCALL(b, getsockname, asa, alen);

        return err;
}

static OSKIT_COMDECL
socket_setsockopt(oskit_socket_t *s, oskit_u32_t level, oskit_u32_t name,
                const void *val, oskit_size_t valsize)
{
        socketwrap_impl_t *b = (socketwrap_impl_t *)(s);
        oskit_error_t    err;

        err = WRAPPERCALL(b, setsockopt,
                          level, name, val, valsize);

        return err;
}

static OSKIT_COMDECL
socket_sendto(oskit_socket_t *s, const void *msg, oskit_size_t len,
                oskit_u32_t flags, const struct oskit_sockaddr *to,
                oskit_size_t tolen, oskit_size_t *retval)
{
        socketwrap_impl_t *b = (socketwrap_impl_t *)(s);
        oskit_error_t    err;

        err = WRAPPERCALL(b, sendto,
                          msg, len, flags, to, tolen, retval);

        return err;
}

static OSKIT_COMDECL
socket_recvfrom(oskit_socket_t *s, void *buf,
                oskit_size_t len, oskit_u32_t flags,
                struct oskit_sockaddr *from, oskit_size_t *fromlen,
                oskit_size_t *retval)
{
        socketwrap_impl_t *b = (socketwrap_impl_t *)(s);
        oskit_error_t    err;

        err = WRAPPERCALL(b, recvfrom,
                          buf, len, flags, from, fromlen, retval);

        return err;
}

static OSKIT_COMDECL
socket_getsockopt(oskit_socket_t *s, oskit_u32_t level, oskit_u32_t name,
                void *val, oskit_size_t *valsize)
{
        socketwrap_impl_t *b = (socketwrap_impl_t *)(s);
        oskit_error_t    err;

        err = WRAPPERCALL(b, getsockopt,
                          level, name, val, valsize);

        return err;
}

static OSKIT_COMDECL
socket_sendmsg(oskit_socket_t *s, const struct oskit_msghdr *msg,
                oskit_u32_t flags, oskit_size_t *retval)
{
        socketwrap_impl_t *b = (socketwrap_impl_t *)(s);
        oskit_error_t    err;

        err = WRAPPERCALL(b, sendmsg, msg, flags, retval);

        return err;
}

static OSKIT_COMDECL
socket_recvmsg(oskit_socket_t *s, struct oskit_msghdr *msg,
                oskit_u32_t flags, oskit_size_t *retval)
{
        socketwrap_impl_t *b = (socketwrap_impl_t *)(s);
        oskit_error_t    err;

        err = WRAPPERCALL(b, recvmsg, msg, flags, retval);

        return err;
}

static OSKIT_COMDECL
socket_getpeername(oskit_socket_t *f,
        struct oskit_sockaddr *asa, oskit_size_t *alen)
{
        socketwrap_impl_t *b = (socketwrap_impl_t *)(f);
        oskit_error_t    err;

        err = WRAPPERCALL(b, getpeername, asa, alen);

        return err;
}

static OSKIT_COMDECL
socket_shutdown(oskit_socket_t *f, oskit_u32_t how)
{
        socketwrap_impl_t *b = (socketwrap_impl_t *)(f);
        oskit_error_t    err;

        err = WRAPPERCALL(b, shutdown, how);

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

/*
 * Wrap a socket_t in before/after brackets.
 */
oskit_error_t
oskit_wrap_socket(oskit_socket_t *in,
	void (*before)(void *), void (*after)(void *), void *cookie,
	oskit_socket_t **out)
{
        socketwrap_impl_t *newsi = malloc(sizeof(*newsi));

        if (newsi == NULL)
                return OSKIT_ENOMEM;

	newsi->count = 1;
	newsi->ioi.ops = &sockops;
	newsi->wrapped = in;
	oskit_socket_addref(in);
	newsi->before = before;
	newsi->after = after;
	newsi->cookie = cookie;
	*out = &newsi->ioi;
	return 0;
}
