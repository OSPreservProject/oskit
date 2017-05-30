/*
 * Copyright (c) 1997-2001 University of Utah and the Flux Group.
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
 * Definitions of the COM-based interfaces for sockets.
 *
 * This file defines the oskit_socket implementation.
 * At that time, it is relatively straightforward,
 * since it is based on the deprecated bsdnet_ interfaces.
 */

#if FLASK
#include <oskit/comsid.h>
#include "netflask.h"
#endif

#ifndef offsetof
#define offsetof(type, member) ((size_t)(&((type *)0)->member))
#endif

#include <oskit/dev/dev.h>
#include <oskit/net/freebsd.h>
#include <oskit/net/socket.h>

#include "sockimpl.h"
#include "mbuf_buf_io.h"

#include <oskit/c/string.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/select.h>
#include <sys/protosw.h>
#include <sys/uio.h>
#include <oskit/dev/freebsd.h>	/* declares xlate_errno */

#include "glue.h"

static inline oskit_error_t
xlaterc(int rc)
{
	return rc ? oskit_freebsd_xlate_errno(rc) : 0;
}


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

        if (memcmp(iid, &oskit_bufio_stream_iid, sizeof(*iid)) == 0) {
                *out_ihandle = &b->iob;
                ++b->count;
                return 0;
	}

        *out_ihandle = NULL;
        return OSKIT_E_NOINTERFACE;
}

static OSKIT_COMDECL
socket_query(oskit_socket_t *f, const struct oskit_guid *iid, void **out_ihandle)
{
        oskit_sockimpl_t *b = (oskit_sockimpl_t *)f;
	return query(b, iid, out_ihandle);
}

#if FLASK
static OSKIT_COMDECL
secsocket_query(oskit_socket_secure_t *f, const struct oskit_guid *iid, void **out_ihandle)
{
	oskit_sockimpl_t *b = (oskit_sockimpl_t*) ((char *) f - 
					   offsetof(oskit_sockimpl_t, sosi));
	return query(b, iid, out_ihandle);
}
#endif

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

#if FLASK
static OSKIT_COMDECL_U
secsocket_addref(oskit_socket_secure_t *f)
{
	oskit_sockimpl_t *b = (oskit_sockimpl_t*) ((char *) f - 
					   offsetof(oskit_sockimpl_t, sosi));
	return addref(b);
}
#endif

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
		struct proc p;
                /* ref count is 0, free ourselves. */
		/*
		 * Note that the socket will linger around after soclose()
		 * make sure that the selinfo records are cleared, since they
		 * would point to freed memory otherwise
		 */
		si->so->so_rcv.sb_sel.si_sel = 0;
		si->so->so_snd.sb_sel.si_sel = 0;
		OSKIT_FREEBSD_CREATE_CURPROC(p);
		soclose(si->so);
		OSKIT_FREEBSD_DESTROY_CURPROC(p);
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

#if FLASK
static OSKIT_COMDECL_U
secsocket_release(oskit_socket_secure_t *f)
{
	oskit_sockimpl_t *b = (oskit_sockimpl_t*) ((char *) f - 
					   offsetof(oskit_sockimpl_t, sosi));
        return release(b);
}
#endif

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

/*** Operations specific to oskit_socket_t ***/
static OSKIT_COMDECL
socket_accept(oskit_socket_t *s, struct oskit_sockaddr *name,
	oskit_size_t *anamelen, struct oskit_socket **newopenso)
{
	oskit_sockimpl_t *b;
	oskit_error_t rc;
	struct	socket *nso;

	rc = bsdnet_accept(((oskit_sockimpl_t*)s)->so,
			   (struct sockaddr *)name, anamelen, &nso);
	if (rc)
		return oskit_freebsd_xlate_errno(rc);

	/* create a new sockimpl */
	b = bsdnet_create_sockimpl(nso);		/* XXX error check */

	*newopenso = &b->ioi;
	return xlaterc(rc);
}

static OSKIT_COMDECL
socket_bind(oskit_socket_t *s, const struct oskit_sockaddr *name,
	oskit_size_t namelen)
{
#if FLASK
        oskit_sockimpl_t *b = (oskit_sockimpl_t *)s;
        security_id_t csid;
	int rc;
	
	CSID(&csid);
	COMMONSOCHECK(csid, b->so, SETLOCAL, &rc);
	if (rc)
		return oskit_freebsd_xlate_errno(rc);
#endif
	return xlaterc(bsdnet_bind(((oskit_sockimpl_t *)s)->so,
				   (struct sockaddr *)name, namelen));
}

static OSKIT_COMDECL
socket_connect(oskit_socket_t *s, const struct oskit_sockaddr *name,
	oskit_size_t namelen)
{
	oskit_sockimpl_t *b = (oskit_sockimpl_t *)s;
#if FLASK
        security_id_t csid;
	int rc;
	
	CSID(&csid);
	COMMONSOCHECK(csid, b->so, SETREMOTE, &rc);
	if (rc)
		return oskit_freebsd_xlate_errno(rc);
#endif
	return xlaterc(bsdnet_connect(b->so,
				      (struct sockaddr *)name, namelen));
}

static OSKIT_COMDECL
socket_listen(oskit_socket_t *s, oskit_u32_t n)
{
	oskit_sockimpl_t *b = (oskit_sockimpl_t *)s;
	struct proc p;
	int    rc;
#if FLASK
        security_id_t csid;
	
	CSID(&csid);
	STRSOCHECK(csid, b->so, LISTEN, &rc);
	if (rc)
		return oskit_freebsd_xlate_errno(rc);

	b->so->so_newconn_sid = b->so->so_sid;
	b->so->so_options &= ~SO_SIDREFLECT;
#endif

	OSKIT_FREEBSD_CREATE_CURPROC(p);
	rc = solisten(b->so, n, &p);
	OSKIT_FREEBSD_DESTROY_CURPROC(p);
	return xlaterc(rc);
}

static OSKIT_COMDECL
socket_getsockname(oskit_socket_t *s, struct oskit_sockaddr *asa,
	oskit_size_t *alen)
{
#if FLASK
        oskit_sockimpl_t *b = (oskit_sockimpl_t *)s;
        security_id_t csid;
	int rc;
	
	CSID(&csid);
	COMMONSOCHECK(csid, b->so, GETLOCAL, &rc);
	if (rc)
		return oskit_freebsd_xlate_errno(rc);
#endif
	return xlaterc(bsdnet_getsockname(((oskit_sockimpl_t *)s)->so,
					  (struct sockaddr *)asa, alen));
}

static OSKIT_COMDECL
socket_setsockopt(oskit_socket_t *s, oskit_u32_t level, oskit_u32_t name,
		const void *val, oskit_size_t valsize)
{
#if FLASK
        oskit_sockimpl_t *b = (oskit_sockimpl_t *)s;
        security_id_t csid;
	int rc;
	
	CSID(&csid);
	if (level == OSKIT_SOL_SOCKET)
		COMMONSOCHECK(csid, b->so, SETOPT, &rc);
	else
		COMMONSOCHECK(csid, b->so, SETPROTOOPT, &rc);
	if (rc)
		return oskit_freebsd_xlate_errno(rc);
#endif
	return xlaterc(bsdnet_setsockopt(((oskit_sockimpl_t *)s)->so,
					 level, name, val, valsize));
}

static OSKIT_COMDECL
socket_sendto(oskit_socket_t *s, const void *msg, oskit_size_t len,
		oskit_u32_t flags, const struct oskit_sockaddr *to,
		oskit_size_t tolen, oskit_size_t *retval)
{
	oskit_sockimpl_t *b = (oskit_sockimpl_t *)s;
#if FLASK
        security_id_t csid;
	int rc;
	
	CSID(&csid);
	COMMONSOCHECK(csid, b->so, SEND, &rc);
	if (rc)
		return oskit_freebsd_xlate_errno(rc);
#endif
	return xlaterc(bsdnet_sendto(b->so,
				     msg, len, flags,
				     (struct sockaddr *)to, tolen, retval));
}

static OSKIT_COMDECL
socket_recvfrom(oskit_socket_t *s, void *buf,
		oskit_size_t len, oskit_u32_t flags,
		struct oskit_sockaddr *from, oskit_size_t *fromlen,
		oskit_size_t *retval)
{
#if FLASK
	oskit_sockimpl_t *b = (oskit_sockimpl_t *)s;
        security_id_t csid;
	int rc;
	
	CSID(&csid);
	COMMONSOCHECK(csid, b->so, RECEIVE, &rc);
	if (rc)
		return oskit_freebsd_xlate_errno(rc);
#endif
	return xlaterc(bsdnet_recvfrom(((oskit_sockimpl_t *)s)->so,
				       buf, len, flags,
				       (struct sockaddr *)from, fromlen,
				       retval));
}

static OSKIT_COMDECL
socket_getsockopt(oskit_socket_t *s, oskit_u32_t level, oskit_u32_t name,
		void *val, oskit_size_t *valsize)
{
#if FLASK
        oskit_sockimpl_t *b = (oskit_sockimpl_t *)s;
        security_id_t csid;
	int rc;
	
	CSID(&csid);
	COMMONSOCHECK(csid, b->so, GETOPT, &rc);
	if (rc)
		return oskit_freebsd_xlate_errno(rc);
#endif
	return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
socket_sendmsg(oskit_socket_t *s, const struct oskit_msghdr *msg,
		oskit_u32_t flags, oskit_size_t *retval)
{
#if FLASK
	oskit_sockimpl_t *b = (oskit_sockimpl_t *)s;
        security_id_t csid;
	int rc;
	
	CSID(&csid);
	COMMONSOCHECK(csid, b->so, SEND, &rc);
	if (rc)
		return oskit_freebsd_xlate_errno(rc);
#endif
	return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
socket_recvmsg(oskit_socket_t *s, struct oskit_msghdr *msg,
		oskit_u32_t flags, oskit_size_t *retval)
{
#if FLASK
	oskit_sockimpl_t *b = (oskit_sockimpl_t *)s;
        security_id_t csid;
	int rc;
	
	CSID(&csid);
	COMMONSOCHECK(csid, b->so, RECEIVE, &rc);
	if (rc)
		return oskit_freebsd_xlate_errno(rc);
#endif
	return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
socket_getpeername(oskit_socket_t *f,
	struct oskit_sockaddr *asa, oskit_size_t *alen)
{
	oskit_sockimpl_t *s = (oskit_sockimpl_t *)f;
#if FLASK
        security_id_t csid;
	int rc;
	
	CSID(&csid);
	COMMONSOCHECK(csid, s->so, GETREMOTE, &rc);
	if (rc)
		return oskit_freebsd_xlate_errno(rc);
#endif
	return xlaterc(bsdnet_getpeername(s->so,
					  (struct sockaddr *)asa, alen));
}

static OSKIT_COMDECL
socket_shutdown(oskit_socket_t *f, oskit_u32_t how)
{
	struct proc p;
	int    rc;
#if FLASK
	oskit_sockimpl_t *b = (oskit_sockimpl_t *)f;
        security_id_t csid;
	
	CSID(&csid);
	switch (how)
	{
	  case 0:
	    COMMONSOCHECK(csid, b->so, DISABLE_RECEIVE, &rc);
	    break;
	  case 1:
	    COMMONSOCHECK(csid, b->so, DISABLE_SEND, &rc);
	    break;
	  case 2:
	    COMMONSOCHECK2(csid, b->so, DISABLE_RECEIVE, DISABLE_SEND, &rc);
	    break;
	  default:
	    return OSKIT_E_INVALIDARG;
	}

	if (rc)
		return oskit_freebsd_xlate_errno(rc);
#endif

	OSKIT_FREEBSD_CREATE_CURPROC(p);
	rc = soshutdown(((oskit_sockimpl_t *)f)->so, how);
	OSKIT_FREEBSD_DESTROY_CURPROC(p);
	return xlaterc(rc);
}


#if FLASK
/*** Operations specific to oskit_socket_secure_t ***/
static OSKIT_COMDECL secsocket_accept(oskit_socket_secure_t *f, 
				      struct oskit_sockaddr *name,
				      oskit_size_t *anamelen,
				      security_id_t *out_peer_sid,
				      struct oskit_socket **newopenso)
{
	oskit_sockimpl_t *s = (oskit_sockimpl_t*) ((char *) f - 
					   offsetof(oskit_sockimpl_t, sosi));
	oskit_error_t rc;


	rc = socket_accept(&s->ioi, name, anamelen, newopenso);
	if (rc)
		return rc;

	*out_peer_sid = s->so->so_peer_sid;
	return 0;
}


static OSKIT_COMDECL
secsocket_connect(oskit_socket_secure_t *f, const struct oskit_sockaddr *name,
		  oskit_size_t namelen, security_id_t peer_sid)
{
	oskit_sockimpl_t *s = (oskit_sockimpl_t*) ((char *) f - 
					   offsetof(oskit_sockimpl_t, sosi));
        security_id_t csid;
	int rc;
	
	CSID(&csid);
	COMMONSOCHECK(csid, s->so, SETREMOTE, &rc);
	if (rc)
		return oskit_freebsd_xlate_errno(rc);

	return xlaterc(bsdnet_connect_secure(s->so,
				     (struct sockaddr *)name, namelen,
				     peer_sid));
}


static OSKIT_COMDECL
secsocket_listen(oskit_socket_secure_t *f, oskit_u32_t n,
		 security_id_t newconn_sid, oskit_u32_t flags)
{
	oskit_sockimpl_t *s = (oskit_sockimpl_t*) ((char *) f - 
					   offsetof(oskit_sockimpl_t, sosi));
        security_id_t csid;
	int rc;
	
	if (newconn_sid && (flags & OSKIT_SO_SIDREFLECT))
		return OSKIT_E_INVALIDARG;

	CSID(&csid);
	STRSOCHECK(csid, s->so, LISTEN, &rc);
	if (rc)
		return oskit_freebsd_xlate_errno(rc);

	if (newconn_sid)
	{
	    STRSOCHECK(newconn_sid, s->so, ACCEPT_ASSOCIATE, &rc);
	    if (rc)
		    return oskit_freebsd_xlate_errno(rc);
	    s->so->so_newconn_sid = newconn_sid;
	    s->so->so_options &= ~SO_SIDREFLECT;
	}
	else if (flags & OSKIT_SO_SIDREFLECT)
	{
	    s->so->so_newconn_sid = SECSID_NULL;
	    s->so->so_options |= SO_SIDREFLECT;
	}
	
	return socket_listen(&s->ioi,n);
}

static OSKIT_COMDECL
secsocket_getsockname(oskit_socket_secure_t *f, struct oskit_sockaddr *asa,
		      oskit_size_t *alen,
		      security_class_t *out_sclass,
		      security_id_t *out_sid)
{
       	oskit_sockimpl_t *s = (oskit_sockimpl_t*) ((char *) f - 
					   offsetof(oskit_sockimpl_t, sosi));

	*out_sclass = s->so->so_sclass;
	*out_sid = s->so->so_sid;
	return socket_getsockname(&s->ioi,asa,alen);
}


static OSKIT_COMDECL
secsocket_getpeername(oskit_socket_secure_t *f,
		      struct oskit_sockaddr *asa, oskit_size_t *alen,
		      security_id_t *out_peer_sid)
{
       	oskit_sockimpl_t *s = (oskit_sockimpl_t*) ((char *) f - 
					   offsetof(oskit_sockimpl_t, sosi));

	*out_peer_sid = s->so->so_peer_sid;
	return socket_getpeername(&s->ioi,asa,alen);
}


static OSKIT_COMDECL
secsocket_sendto(oskit_socket_secure_t *f, const void *msg, oskit_size_t len,
		 oskit_u32_t flags, const struct oskit_sockaddr *to,
		 oskit_size_t tolen, oskit_size_t *retval,
		 security_id_t dso_sid, security_id_t msg_sid)
{
       	oskit_sockimpl_t *s = (oskit_sockimpl_t*) ((char *) f - 
					   offsetof(oskit_sockimpl_t, sosi));
        security_id_t csid;
	int rc;
	
	CSID(&csid);
	COMMONSOCHECK(csid, s->so, SEND, &rc);
	if (rc)
		return oskit_freebsd_xlate_errno(rc);

	COMMONSOCHECK(msg_sid, s->so, SEND_ASSOCIATE, &rc);
	if (rc)
		return oskit_freebsd_xlate_errno(rc);	

	return xlaterc(bsdnet_sendto_secure(s->so,
				     msg, len, flags,
				     (struct sockaddr *)to, tolen, retval,
					    dso_sid, msg_sid));
}


static OSKIT_COMDECL
secsocket_recvfrom(oskit_socket_secure_t *s, void *buf,
		   oskit_size_t len, oskit_u32_t flags,
		   struct oskit_sockaddr *from, oskit_size_t *fromlen,
		   oskit_size_t *retval, 
		   security_id_t *out_sso_sid,
		   security_id_t *out_msg_sid)
{
	oskit_sockimpl_t *b = (oskit_sockimpl_t*) ((char *) s - 
					   offsetof(oskit_sockimpl_t, sosi));
        security_id_t csid;
	int rc;
	
	CSID(&csid);
	COMMONSOCHECK(csid, b->so, RECEIVE, &rc);
	if (rc)
		return oskit_freebsd_xlate_errno(rc);

	return xlaterc(bsdnet_recvfrom_secure(b->so,
				       buf, len, flags,
				       (struct sockaddr *)from, fromlen,
				       retval,out_sso_sid,out_msg_sid));
}
#endif

/*******************************************************/
/******* Implementation of the oskit_stream_t if ********/
/*******************************************************/

/*** Operations inherited from oskit_stream interface ***/
static OSKIT_COMDECL
opensocket_read(oskit_stream_t *f, void *buf, oskit_u32_t len,
				oskit_u32_t *out_actual)
{
	oskit_sockimpl_t *s = (oskit_sockimpl_t *)(f-1);
#if FLASK
        security_id_t csid;
	int rc;
	
	CSID(&csid);
	COMMONSOCHECK(csid, s->so, RECEIVE, &rc);
	if (rc)
		return oskit_freebsd_xlate_errno(rc);
#endif
	return xlaterc(bsdnet_read(s->so, buf, len, out_actual));
}

static OSKIT_COMDECL
opensocket_write(oskit_stream_t *f, const void *buf,
				 oskit_u32_t len, oskit_u32_t *out_actual)
{
	oskit_sockimpl_t *s = (oskit_sockimpl_t *)(f-1);
#if FLASK
        security_id_t csid;
	int rc;
	
	CSID(&csid);
	COMMONSOCHECK(csid, s->so, SEND, &rc);
	if (rc)
		return oskit_freebsd_xlate_errno(rc);
#endif
	return xlaterc(bsdnet_write(s->so, buf, len, out_actual));
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
get_socket_conditions(struct socket *so)
{
	oskit_u32_t res = 0;
        if (soreadable(so))
                res |= OSKIT_ASYNCIO_READABLE;
        if (sowriteable(so))
                res |= OSKIT_ASYNCIO_WRITABLE;
        if (so->so_oobmark || (so->so_state & SS_RCVATMARK))
                res |= OSKIT_ASYNCIO_EXCEPTION;
	return res;
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
	struct socket   *so = si->so;
        oskit_u32_t      res = 0;
        register int    s = splnet();

	res = get_socket_conditions(so);
        splx(s);
        return (res);
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
	struct socket   *so = si->so;
	oskit_s32_t	cond;
	struct proc 	p;
        register int    s;

	OSKIT_FREEBSD_CREATE_CURPROC(p);
        s = splnet();
	cond = get_socket_conditions(so);

	/* for read and exceptional conditions */
	if (mask & (OSKIT_ASYNCIO_READABLE | OSKIT_ASYNCIO_EXCEPTION))
	{
		oskit_listener_mgr_add(si->readers, l);
		curproc->p_sel = si->readers;
		selrecord(curproc, &so->so_rcv.sb_sel);
		curproc->p_sel = 0;
		so->so_rcv.sb_flags |= SB_SEL;
	}

	/* for write */
	if (mask & OSKIT_ASYNCIO_WRITABLE)
	{
		oskit_listener_mgr_add(si->writers, l);
		curproc->p_sel = si->writers;
		selrecord(curproc, &so->so_snd.sb_sel);
		curproc->p_sel = 0;
		so->so_snd.sb_flags |= SB_SEL;
	}
        splx(s);
	OSKIT_FREEBSD_DESTROY_CURPROC(p);
        return (cond);
}

/*
 * Remove a previously registered listener callback object.
 * Returns an error if the specified callback has not been registered.
 */
static OSKIT_COMDECL
asyncio_remove_listener(oskit_asyncio_t *f, struct oskit_listener *l0)
{
	oskit_sockimpl_t *si = (oskit_sockimpl_t *)(f-2);
	oskit_error_t rc1, rc2;

	/*
	 * protect against interrupts, since clients might call
	 * remove_listener out of the asynciolistener callback
	 */
        register int    s = splnet();

	/*
	 * we don't know where was added - if at all - so let's check
	 * both lists
	 *
	 * turn off notifications if no listeners left
	 */
	rc1 = oskit_listener_mgr_remove(si->readers, l0);
	if (oskit_listener_mgr_count(si->readers) == 0) {
		si->so->so_rcv.sb_sel.si_sel = 0;
		si->so->so_rcv.sb_flags &= ~SB_SEL;
	}

	rc2 = oskit_listener_mgr_remove(si->writers, l0);
	if (oskit_listener_mgr_count(si->writers) == 0) {
		si->so->so_snd.sb_sel.si_sel = 0;
		si->so->so_snd.sb_flags &= ~SB_SEL;
	}

	splx(s);

	/* flag error if both removes failed */
	return (rc1 && rc2) ? OSKIT_E_INVALIDARG : 0;	/* is that right ? */
}

/*
 * return the number of bytes that can be read, basically ioctl(FIONREAD)
 */
static OSKIT_COMDECL
asyncio_readable(oskit_asyncio_t *f)
{
	oskit_sockimpl_t *si = (oskit_sockimpl_t *)(f-2);
	/* see /usr/src/sys/kern/sys_socket.c */
	return si->so->so_rcv.sb_cc;
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
	oskit_error_t	rc;
	struct socket *so = si->so;
	int	mlen = 1440;	/* ???: 1526 - 12 - 74 */
	struct proc  p;
	struct mbuf *m;
	struct uio auio;

	OSKIT_FREEBSD_CREATE_CURPROC(p);
	auio.uio_resid = mlen;
	auio.uio_procp = &p;
	rc = so->so_proto->pr_usrreqs->pru_soreceive(so, (struct sockaddr **)0,
			&auio, (struct mbuf **)&m,
			(struct mbuf **)0, (int *)0);

	*bytes = mlen - auio.uio_resid;
	if (*bytes > 0)
		*buf = mbuf_bufio_create_instance(m, 0);
	else
		/* XXX violates COM rules.
		   should return an error code instead ??? */
		*buf = 0;

	OSKIT_FREEBSD_DESTROY_CURPROC(p);
	return xlaterc(rc);
}

static OSKIT_COMDECL
bufio_stream_write(oskit_bufio_stream_t *f,
	struct oskit_bufio *buf, oskit_size_t offset)
{
	oskit_sockimpl_t *si = (oskit_sockimpl_t *)(f-3);
	oskit_error_t	rc;
	struct socket *so = si->so;
	struct proc  p;
	struct mbuf *m;
	oskit_off_t   size;
	oskit_size_t  payload;
	void	      *data;

#if FLASK
        security_id_t csid;
	
	CSID(&csid);
	COMMONSOCHECK(csid, so, SEND, &rc);
	if (rc)
		return oskit_freebsd_xlate_errno(rc);
#endif

	OSKIT_FREEBSD_CREATE_CURPROC(p);

	rc = oskit_bufio_getsize(buf, &size);
	if (rc)
		goto bailout;

	/* Don't bother if I can't map */
	rc = oskit_bufio_map(buf, &data, 0, (oskit_size_t)size);
	if (rc)
		goto bailout;

	payload = size - offset;
	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == 0) {
		rc = OSKIT_ENOMEM;
		goto bailout;
	}

	/* let's fake an external mbuf, see net_receive.c */
        m->m_pkthdr.rcvif = (struct ifnet *)0;
        m->m_pkthdr.len = 0;

        /*
         * here we go, manually assembling our new external mbuf
         */
        m->m_ext.ext_buf = data;
        m->m_ext.ext_size = size;
        m->m_data = data + offset;
        m->m_len = payload;

	oskit_bufio_addref(buf);
        m->m_ext.ext_bufio = buf;
        m->m_flags |= M_EXT;

	rc = so->so_proto->pr_usrreqs->pru_sosend(so, (struct sockaddr *)0, (struct uio *)0,
			m, (struct mbuf *)0, 0, &p);

	/* unmap ??? */
bailout:
	OSKIT_FREEBSD_DESTROY_CURPROC(p);
	return xlaterc(rc);
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

#if FLASK
static struct oskit_socket_secure_ops secsockops =
{
	secsocket_query,
	secsocket_addref,
	secsocket_release,
	secsocket_accept,
	secsocket_connect,
	secsocket_listen,
	secsocket_getsockname,
	secsocket_getpeername,
	secsocket_sendto,
	secsocket_recvfrom
};
#endif

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
 * create the COM part of a new sockimpl object
 */
oskit_sockimpl_t *
bsdnet_create_sockimpl(struct socket *so)
{
        oskit_sockimpl_t *si = osenv_mem_alloc(sizeof(*si), 0, 0);
        if (si == NULL)
                return NULL;
        memset(si, 0, sizeof(*si));

        si->count = 1;
        si->ioi.ops = &sockops;
        si->ios.ops = &opensockops;
        si->ioa.ops = &asyncioops;
        si->iob.ops = &bufio_streamops;
#if FLASK
	si->sosi.ops = &secsockops;
#endif	
	
	si->so = so;
	si->readers = oskit_create_listener_mgr((oskit_iunknown_t *)&si->ioa);
	si->writers = oskit_create_listener_mgr((oskit_iunknown_t *)&si->ioa);
	return si;
}

/* End of file. */

