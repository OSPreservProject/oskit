/*
 * Copyright (c) 1998, 1999, 2000, 2001 University of Utah and the Flux Group.
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
 * Implementation of socket factory and socket based on NATIVEOS() functions.
 *
 * This implementation is incomplete (asyncio listeners are missing),
 * untested and mostly likely not working.
 *
 * Use
 *
 *   oskit_error_t
 *   oskit_native_net_init(oskit_socket_factory_t **f);
 *
 * to access it.
 */
/*
 * OS Kit includes
 */
#include <oskit/dev/dev.h>
#include <oskit/net/socket.h>
#include <oskit/io/asyncio.h>
#include <oskit/io/bufio_stream.h>
#include <oskit/com/listener_mgr.h>
#include <oskit/clientos.h>
#include <oskit/net/socket.h>

#include "native.h"
#include "support.h"

/*
 * FreeBSD includes
 */
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <string.h>

#include <errno.h>

/*
 * socket implementation
 */
struct oskit_sockimpl {
        oskit_socket_t  ioi;                    /* COM I/O interface Nr. 1 */
        oskit_stream_t  ios;                    /* COM I/O interface Nr. 2 */
        oskit_asyncio_t ioa;                    /* COM I/O interface Nr. 3 */
        unsigned        count;                  /* reference count */

        int 		fd;                    /* native OS fd */
	osenv_sleeprec_t    sleeprec;	/* For non-blocking I/O */
	oskit_u32_t	    selmask;	/* Select mask */
        struct listener_mgr *readers;   /* listeners for asyncio READ */
        struct listener_mgr *writers;   /* listeners for asyncio WRITE*/
};
typedef struct oskit_sockimpl oskit_sockimpl_t;

static oskit_sockimpl_t * create_sockimpl(int fd);

/*
 * implementations of query, addref and release dealing with
 * different COM interfaces
 */
/*
 * query
 */
static oskit_error_t
query(oskit_sockimpl_t *b, const struct oskit_guid *iid, void **out_ihandle)
{
        if (b == NULL)
                osenv_panic("%s:%d: null oskit_sockimpl_t", __FILE__, __LINE__);
        if (b->count == 0)
                osenv_panic("%s:%d: bad count", __FILE__, __LINE__);

        if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
            memcmp(iid, &oskit_posixio_iid, sizeof(*iid)) == 0 ||
            memcmp(iid, &oskit_socket_iid, sizeof(*iid)) == 0) {
                *out_ihandle = &b->ioi;
                ++b->count;
                return 0;
        }

        if (memcmp(iid, &oskit_stream_iid, sizeof(*iid)) == 0) {
                *out_ihandle = &b->ios;
                ++b->count;
                return 0;
        }

        if (memcmp(iid, &oskit_asyncio_iid, sizeof(*iid)) == 0) {
                *out_ihandle = &b->ioa;
                ++b->count;
                return 0;
        }

        *out_ihandle = NULL;
        return OSKIT_E_NOINTERFACE;
}

static OSKIT_COMDECL
socket_query(oskit_socket_t *f, const struct oskit_guid *iid, void **out_ihandle
)
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

/*
 * addref
 */
static oskit_u32_t
addref(oskit_sockimpl_t *b)
{
        if (b == NULL)
                osenv_panic("%s:%d: null oskit_sockimpl_t", __FILE__, __LINE__);
        if (b->count == 0)
                osenv_panic("%s:%d: bad count", __FILE__, __LINE__);

        return ++b->count;
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

/*
 * release
 */
static oskit_u32_t
release(oskit_sockimpl_t *si)
{
        unsigned newcount;

        if (si == NULL)
                osenv_panic("%s:%d: null sockimpl_t", __FILE__, __LINE__);
        if (si->count == 0)
                osenv_panic("%s:%d: bad count", __FILE__, __LINE__);

        if ((newcount = --si->count) == 0) {
		oskitunix_unregister_async_fd(si->fd);
		NATIVEOS(close)(si->fd);
                oskit_destroy_listener_mgr(si->readers);
                oskit_destroy_listener_mgr(si->writers);
                osenv_mem_free(si, 0, sizeof(*si));
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

/*******************************************************/
/******* Implementation of the oskit_socket_t if ********/
/*******************************************************/

/*** Operations inherited from oskit_posixio_t ***/
static OSKIT_COMDECL
socket_stat(oskit_socket_t *f, struct oskit_stat *out_stats)
{
	/* XXX do a fstat and convert the values */
        memset(out_stats, 0, sizeof *out_stats);
        out_stats->mode = OSKIT_S_IFSOCK;
        return 0;
}

static OSKIT_COMDECL
socket_setstat(oskit_socket_t *f, oskit_u32_t mask,
        const struct oskit_stat *stats)
{
        return OSKIT_ENOTSUP;
}

static OSKIT_COMDECL
socket_pathconf(oskit_socket_t *f, oskit_s32_t option, oskit_s32_t *out_val)
{
        return OSKIT_ENOTSUP;
}

#ifdef NATIVE_HAVE_SA_LEN
#define SA_FIX_OUT(sa, salen)	((void)0)
#define	SA_IN(sa, salen)	struct sockaddr *const SA = (void *)(sa);
#else
#define SA_FIX_OUT(sa, len) \
	((((struct oskit_sockaddr *) (sa))->sa_family = \
	  ((struct sockaddr *) (sa))->sa_family), \
	 ((struct oskit_sockaddr *) (sa))->sa_len = (len))
#define SA_IN(sa, salen)						      \
	struct sockaddr _sa, *const SA = &_sa;				      \
	assert((salen) <= sizeof _sa);					      \
	_sa.sa_family = (sa)->sa_family;				      \
	memcpy(_sa.sa_data, (sa)->sa_data,				      \
	       (salen) - offsetof(struct oskit_sockaddr, sa_data))
#endif

/*** Operations specific to oskit_socket_t ***/
static OSKIT_COMDECL
socket_accept(oskit_socket_t *s, struct oskit_sockaddr *name,
        oskit_size_t *anamelen, struct oskit_socket **newopenso)
{
        oskit_sockimpl_t *b;
	int	nfd;

	assert(offsetof(struct oskit_sockaddr, sa_data) ==
	       offsetof(struct sockaddr, sa_data));

	if ((nfd = oskitunix_threaded_accept(((oskit_sockimpl_t*)s)->fd,
				   (struct sockaddr *)name, anamelen)) < 0)
		return native_to_oskit_error(NATIVEOS(errno));

	SA_FIX_OUT(name, *anamelen);

        /* create a new sockimpl */
        b = create_sockimpl(nfd);               /* XXX error check */

        *newopenso = &b->ioi;
        return 0;
}

static OSKIT_COMDECL
socket_bind(oskit_socket_t *s, const struct oskit_sockaddr *name,
        oskit_size_t namelen)
{
	SA_IN(name, namelen);
        if (NATIVEOS(bind)(((oskit_sockimpl_t *)s)->fd,
			   SA, namelen) < 0)
		return native_to_oskit_error(NATIVEOS(errno));

	return 0;
}

static OSKIT_COMDECL
socket_connect(oskit_socket_t *s, const struct oskit_sockaddr *name,
        oskit_size_t namelen)
{
        oskit_sockimpl_t *b = (oskit_sockimpl_t *)s;
	SA_IN(name, namelen);

	if (oskitunix_threaded_connect(b->fd, SA, namelen) < 0)
		return native_to_oskit_error(NATIVEOS(errno));

        return 0;
}

static OSKIT_COMDECL
socket_listen(oskit_socket_t *s, oskit_u32_t n)
{
	if (NATIVEOS(listen)(((oskit_sockimpl_t *)s)->fd, n) < 0)
	    	return native_to_oskit_error(NATIVEOS(errno));

	return 0;
}

static OSKIT_COMDECL
socket_getsockname(oskit_socket_t *s, struct oskit_sockaddr *asa,
        oskit_size_t *alen)
{
    	if (NATIVEOS(getsockname)(((oskit_sockimpl_t *)s)->fd,
				  (struct sockaddr *)asa, alen) < 0)
	    	return native_to_oskit_error(NATIVEOS(errno));
	SA_FIX_OUT(asa, *alen);
	return 0;
}

/* XXX translate sockopts! */
static OSKIT_COMDECL
socket_setsockopt(oskit_socket_t *s, oskit_u32_t level, oskit_u32_t name,
                const void *val, oskit_size_t valsize)
{
    	if (NATIVEOS(setsockopt)(((oskit_sockimpl_t *)s)->fd,
				     level, name, val, valsize) < 0)
	    	return native_to_oskit_error(NATIVEOS(errno));

	return 0;

}

static OSKIT_COMDECL
socket_sendto(oskit_socket_t *s, const void *msg, oskit_size_t len,
                oskit_u32_t flags, const struct oskit_sockaddr *to,
                oskit_size_t tolen, oskit_size_t *retval)
{
	int		  count;
	SA_IN(to, tolen);

	if ((count = oskitunix_threaded_sendto(((oskit_sockimpl_t *)s)->fd,
                                     msg, len, flags,
                                     SA, tolen)) < 0)
		return native_to_oskit_error(NATIVEOS(errno));

	*retval = (oskit_size_t) count;
	return 0;
}

static OSKIT_COMDECL
socket_recvfrom(oskit_socket_t *s, void *buf,
                oskit_size_t len, oskit_u32_t flags,
                struct oskit_sockaddr *from, oskit_size_t *fromlen,
                oskit_size_t *retval)
{
	int		  count;

	if ((count = oskitunix_threaded_recvfrom(((oskit_sockimpl_t *)s)->fd,
                                       buf, len, flags,
                                       (struct sockaddr *)from, fromlen)) < 0)
		return native_to_oskit_error(NATIVEOS(errno));
	SA_FIX_OUT(from, *fromlen);
	*retval = (oskit_size_t) count;
	return 0;
}

static OSKIT_COMDECL
socket_getsockopt(oskit_socket_t *s, oskit_u32_t level, oskit_u32_t name,
                void *val, oskit_size_t *valsize)
{
        return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
socket_sendmsg(oskit_socket_t *s, const struct oskit_msghdr *msg,
                oskit_u32_t flags, oskit_size_t *retval)
{
        return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
socket_recvmsg(oskit_socket_t *s, struct oskit_msghdr *msg,
                oskit_u32_t flags, oskit_size_t *retval)
{
        return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
socket_getpeername(oskit_socket_t *f,
        struct oskit_sockaddr *asa, oskit_size_t *alen)
{
        oskit_sockimpl_t *s = (oskit_sockimpl_t *)f;
        if (NATIVEOS(getpeername)(s->fd, (struct sockaddr *)asa, alen) < 0)
	    	return native_to_oskit_error(NATIVEOS(errno));
	SA_FIX_OUT(asa, *alen);
	return 0;
}

static OSKIT_COMDECL
socket_shutdown(oskit_socket_t *f, oskit_u32_t how)
{
        if (NATIVEOS(shutdown)(((oskit_sockimpl_t *)f)->fd, how) < 0)
	    	return native_to_oskit_error(NATIVEOS(errno));

	return 0;
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
	int		  count;

	if ((count = oskitunix_threaded_read(s->fd, buf, len)) < 0)
		return native_to_oskit_error(NATIVEOS(errno));

	*out_actual = count;
	return 0;
}

static OSKIT_COMDECL
opensocket_write(oskit_stream_t *f, const void *buf,
                                 oskit_u32_t len, oskit_u32_t *out_actual)
{
        oskit_sockimpl_t *s = (oskit_sockimpl_t *)(f-1);
	int		  count;

	if ((count = oskitunix_threaded_write(s->fd, buf, len)) < 0)
		return native_to_oskit_error(NATIVEOS(errno));

	*out_actual = count;
	return 0;
}

static OSKIT_COMDECL
opensocket_seek(oskit_stream_t *f, oskit_s64_t ofs,
        oskit_seek_t whence, oskit_u64_t *out_newpos)
{
        return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
opensocket_setsize(oskit_stream_t *f, oskit_u64_t new_size)
{
        return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
opensocket_copyto(oskit_stream_t *f, oskit_stream_t *dst,
        oskit_u64_t size,
        oskit_u64_t *out_read,
        oskit_u64_t *out_written)
{
        return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
opensocket_commit(oskit_stream_t *f, oskit_u32_t commit_flags)
{
        return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
opensocket_revert(oskit_stream_t *f)
{
        return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
opensocket_lock_region(oskit_stream_t *f,
        oskit_u64_t offset, oskit_u64_t size, oskit_u32_t lock_type)
{
        return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
opensocket_unlock_region(oskit_stream_t *f,
        oskit_u64_t offset, oskit_u64_t size, oskit_u32_t lock_type)
{
        return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
opensocket_stat(oskit_stream_t *f, oskit_stream_stat_t *out_stat,
        oskit_u32_t stat_flags)
{
        /* this should be implemented soon */
        return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
opensocket_clone(oskit_stream_t *f, oskit_stream_t **out_stream)
{
        return OSKIT_E_NOTIMPL;
}

/*******************************************************/
/******* Implementation of the oskit_asyncio_t if *******/
/*******************************************************/

/*
 * return a mask with all conditions that currently apply to that socket
 * must be called with splnet()!
 */
static oskit_u32_t
get_socket_conditions(int fd)
{
        oskit_u32_t res = 0;
	int r;
	struct timeval nulltime = { 0, 0 };
	fd_set	in, out, err;

	FD_ZERO(&in);
	FD_ZERO(&out);
	FD_ZERO(&err);
	FD_SET(fd, &in);
	FD_SET(fd, &out);
	FD_SET(fd, &err);

	/* poll socket */
	r = NATIVEOS(select) (fd + 1, &in, &out, &err, &nulltime);
	if (r < 1)
		return res;

	if (FD_ISSET(fd, &in))
                res |= OSKIT_ASYNCIO_READABLE;
	if (FD_ISSET(fd, &out))
                res |= OSKIT_ASYNCIO_WRITABLE;
	if (FD_ISSET(fd, &err))
                res |= OSKIT_ASYNCIO_EXCEPTION;

        return res;
}

/*
 * Callback for asyncio interface. Determine which conditions are
 * ready and requested, and initiate those callbacks.
 */
static void
asyncio_callback(void *arg)
{
        oskit_sockimpl_t *si = (oskit_sockimpl_t *) arg;
	oskit_u32_t	 ready;
	unsigned	 iotype = 0;

	ready = si->selmask & get_socket_conditions(si->fd);

        /* for read and exceptional conditions */
        if (ready & (OSKIT_ASYNCIO_READABLE | OSKIT_ASYNCIO_EXCEPTION)) {
                oskit_listener_mgr_notify(si->readers);
		iotype |= IOTYPE_READ;
	}

        /* for write */
        if (ready & OSKIT_ASYNCIO_WRITABLE) {
                oskit_listener_mgr_notify(si->writers);
		iotype |= IOTYPE_WRITE;
        }
}

/*
 * Poll for currently pending asynchronous I/O conditions.
 * If successful, returns a mask of the OSKIT_ASYNC_IO_* flags above,
 * indicating which conditions are currently present.
 */
static OSKIT_COMDECL
asyncio_poll(oskit_asyncio_t *f)
{
        oskit_sockimpl_t *si = (oskit_sockimpl_t *)(f-2);
	oskit_u32_t	 cond;
	int		 enabled;

	enabled = osenv_intr_save_disable();
	cond = get_socket_conditions(si->fd);
	if (enabled)
		osenv_intr_enable();

	return cond;
}

/*
 * Add a callback object (a "listener" for async I/O events).
 * When an event of interest occurs on this I/O object
 * (i.e., when one of the three I/O conditions becomes true),
 * all registered listeners will be called.
 * Also, if successful, this method returns a mask
 * describing which of the OSKIT_ASYNC_IO_* conditions are already true,
 * which the caller must check in order to avoid missing events
 * that occur just before the listener is registered.
 */
static OSKIT_COMDECL
asyncio_add_listener(oskit_asyncio_t *f, struct oskit_listener *l,
        oskit_s32_t mask)
{
        oskit_sockimpl_t *si = (oskit_sockimpl_t *)(f-2);
        oskit_s32_t     cond;
	int		enabled;
	unsigned	iotype = 0;

	enabled = osenv_intr_save_disable();

        cond = get_socket_conditions(si->fd);

        /* for read and exceptional conditions */
        if (mask & (OSKIT_ASYNCIO_READABLE | OSKIT_ASYNCIO_EXCEPTION)) {
                oskit_listener_mgr_add(si->readers, l);
		iotype |= IOTYPE_READ;
	}

        /* for write */
        if (mask & OSKIT_ASYNCIO_WRITABLE) {
                oskit_listener_mgr_add(si->writers, l);
		iotype |= IOTYPE_WRITE;
        }

	si->selmask |= mask;
	oskitunix_register_async_fd(si->fd, iotype, asyncio_callback, si);

	if (enabled)
		osenv_intr_enable();
        return cond;
}

/*
 * Remove a previously registered listener callback object.
 * Returns an error if the specified callback has not been registered.
 */
static OSKIT_COMDECL
asyncio_remove_listener(oskit_asyncio_t *f, struct oskit_listener *l0)
{
        oskit_sockimpl_t *si = (oskit_sockimpl_t *)(f-2);
	int		 enabled;
	oskit_error_t	 rc1, rc2;

        /*
         * protect against interrupts, since clients might call
         * remove_listener out of the asynciolistener callback
         */
	enabled = osenv_intr_save_disable();

        /*
         * we don't know where was added - if at all - so let's check
         * both lists
         *
         * turn off notifications if no listeners left
         */
        rc1 = oskit_listener_mgr_remove(si->readers, l0);
        if (oskit_listener_mgr_count(si->readers) == 0)
		si->selmask &=
			~(OSKIT_ASYNCIO_READABLE | OSKIT_ASYNCIO_EXCEPTION);

        rc2 = oskit_listener_mgr_remove(si->writers, l0);
        if (oskit_listener_mgr_count(si->writers) == 0)
		si->selmask &= ~(OSKIT_ASYNCIO_WRITABLE);

	if (! si->selmask)
		oskitunix_unregister_async_fd(si->fd);

	if (enabled)
		osenv_intr_enable();

        /* flag error if both removes failed */
        return (rc1 && rc2) ? OSKIT_E_INVALIDARG : 0;   /* is that right ? */
}

/*
 * return the number of bytes that can be read, basically ioctl(FIONREAD)
 */
static OSKIT_COMDECL
asyncio_readable(oskit_asyncio_t *f)
{
        oskit_sockimpl_t *si = (oskit_sockimpl_t *)(f-2);
	int b;
	int rc;
	rc = NATIVEOS(ioctl)(si->fd, FIONREAD, &b);
	return b;	/* XXX ??? */
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

/*
 * create the COM part of a new sockimpl object
 */
static oskit_sockimpl_t *
create_sockimpl(int fd)
{
	int rc;

        oskit_sockimpl_t *si = osenv_mem_alloc(sizeof(*si), 0, 0);

        if (si == NULL)
                return NULL;
        memset(si, 0, sizeof(*si));

        si->count = 1;
        si->ioi.ops = &sockops;
        si->ios.ops = &opensockops;
        si->ioa.ops = &asyncioops;

        si->fd = fd;
        si->readers = oskit_create_listener_mgr((oskit_iunknown_t *)&si->ioa);
        si->writers = oskit_create_listener_mgr((oskit_iunknown_t *)&si->ioa);

	rc = oskitunix_set_async_fd(fd);
	if (rc)
                osenv_panic("%s:%d: "
			    "OSKit/UNIX emulation fault: cannot make fd (%d)"
			    " async/non-blocking: native errno=%d\n",
			    __FILE__, __LINE__,
			    fd, rc);

        return si;
}

/***************************************************************************/
/*
 * methods of socket_factories
 */
static OSKIT_COMDECL
socket_factory_query(oskit_socket_factory_t *b,
        const struct oskit_guid *iid, void **out_ihandle)
{
        if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
            memcmp(iid, &oskit_socket_factory_iid, sizeof(*iid)) == 0) {
                *out_ihandle = b;
                return 0;
        }
        *out_ihandle = NULL;
        return OSKIT_E_NOINTERFACE;
}

static OSKIT_COMDECL_U
socket_factory_addref(oskit_socket_factory_t *b)
{
        return 1;
}

static OSKIT_COMDECL_U
socket_factory_release(oskit_socket_factory_t *b)
{
        return 1;
}

/*
 * function to create a socket - analog to socket(2)
 */
static OSKIT_COMDECL
socket_factory_create(
        oskit_socket_factory_t *factory,
        oskit_u32_t domain,
        oskit_u32_t type, oskit_u32_t protocol, oskit_socket_t **aso)
{
        oskit_sockimpl_t *b;
	int fd;

        fd = NATIVEOS(socket)(domain, type, protocol);
        if (fd < 0)
                return native_to_oskit_error(NATIVEOS(errno));

        b = create_sockimpl(fd);
        *aso = &b->ioi;
        return 0;
}

/*
 * function to create a pair of connected sockets - analog to socketpair(2)
 */
static OSKIT_COMDECL
socket_factory_create_pair(
        oskit_socket_factory_t *factory,
        oskit_u32_t domain,
        oskit_u32_t type, oskit_u32_t protocol,
	oskit_socket_t **aso1, oskit_socket_t **aso2)
{
        oskit_sockimpl_t *b;
	int fds[2];

        if (NATIVEOS(socketpair)(domain, type, protocol, fds) < 0);
                return native_to_oskit_error(NATIVEOS(errno));

        b = create_sockimpl(fds[0]);
        *aso1 = &b->ioi;
        b = create_sockimpl(fds[1]);
        *aso2 = &b->ioi;
        return 0;
}

static struct oskit_socket_factory_ops sf_ops = {
        socket_factory_query,
        socket_factory_addref,
        socket_factory_release,
        socket_factory_create,
        socket_factory_create_pair
};

static
struct oskit_socket_factory oskit_native_socket_factory = { &sf_ops };

oskit_error_t
oskit_native_net_init(oskit_socket_factory_t **f)
{
	char hostname[256];

	NATIVEOS(gethostname)(hostname, 256);
	oskit_clientos_sethostname(hostname, 256);
        *f = &oskit_native_socket_factory;
        return 0;
}
