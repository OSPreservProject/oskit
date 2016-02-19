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
 * Copyright (c) 1982, 1986, 1989, 1990, 1993
 *      The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

/*
 * bsdnet_recvfrom() - derived from code in /usr/src/sys/kern/uipc_syscalls.c
 */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/signalvar.h>
#include <sys/protosw.h>
#include <sys/uio.h>

#include <oskit/net/freebsd.h>

#include "glue.h"

struct recvfrom_args {
	int	s;
	caddr_t	buf;
	size_t	len;
	int	flags;
	caddr_t	from;
	int	*fromlenaddr;
};

static int
#if FLASK
bsdnet_recvit_secure(so, mp, namelenp, retsize, out_sso_sid, out_msg_sid)
#else
bsdnet_recvit(so, mp, namelenp, retsize)
#endif
	struct socket *so;
	register struct msghdr *mp;
	caddr_t namelenp;
	int *retsize;
#if FLASK
        security_id_t *out_sso_sid, *out_msg_sid;
#endif
{
	struct uio auio;
	register struct iovec *iov;
	register int i;
	int len, error;
	struct proc p;
	struct mbuf *m, *control = 0;
	caddr_t ctlbuf;
	struct sockaddr *fromsa = 0;

	OSKIT_FREEBSD_CREATE_CURPROC(p);
	auio.uio_iov = mp->msg_iov;
	auio.uio_iovcnt = mp->msg_iovlen;
	auio.uio_segflg = UIO_USERSPACE;
	auio.uio_rw = UIO_READ;
	auio.uio_procp = &p;
	auio.uio_offset = 0;			/* XXX (says BSD) */
	auio.uio_resid = 0;
	iov = mp->msg_iov;
	for (i = 0; i < mp->msg_iovlen; i++, iov++) {
		if ((auio.uio_resid += iov->iov_len) < 0) {
			error = (EINVAL);
			goto out;
		}
	}
	len = auio.uio_resid;
#if FLASK
	error = so->so_proto->pr_usrreqs->pru_soreceive_secure(so, &fromsa, &auio,
	    (struct mbuf **)0, mp->msg_control ? &control : (struct mbuf **)0,
	    &mp->msg_flags, out_sso_sid, out_msg_sid);
#else
	error = so->so_proto->pr_usrreqs->pru_soreceive(so, &fromsa, &auio,
	    (struct mbuf **)0, mp->msg_control ? &control : (struct mbuf **)0,
	    &mp->msg_flags);
#endif
	if (error) {
		if (auio.uio_resid != len && (error == ERESTART ||
		    error == EINTR || error == EWOULDBLOCK))
			error = 0;
	}
	if (error)
		goto out;
	*retsize = len - auio.uio_resid;
	if (mp->msg_name) {
		len = mp->msg_namelen;
		if (len <= 0 || fromsa == 0)
			len = 0;
		else {
#ifndef MIN
#define MIN(a,b) ((a)>(b)?(b):(a))
#endif
			/* save sa_len before it is destroyed by MSG_COMPAT */
			len = MIN(len, fromsa->sa_len);
#ifdef COMPAT_OLDSOCK 
			if (mp->msg_flags & MSG_COMPAT)
				((struct osockaddr *)fromsa)->sa_family =
				    fromsa->sa_family;
#endif
			error = copyout(fromsa,
			    (caddr_t)mp->msg_name, (unsigned)len);
			if (error)
				goto out;
		}
		mp->msg_namelen = len;
		if (namelenp &&
		    (error = copyout((caddr_t)&len, namelenp, sizeof (int)))) {
#ifdef COMPAT_OLDSOCK
			if (mp->msg_flags & MSG_COMPAT)
				error = 0;	/* old recvfrom didn't check */
			else
#endif
			goto out;
		}
	}
	if (mp->msg_control) {
#ifdef COMPAT_OLDSOCK
		/*
		 * We assume that old recvmsg calls won't receive access
		 * rights and other control info, esp. as control info
		 * is always optional and those options didn't exist in 4.3.
		 * If we receive rights, trim the cmsghdr; anything else
		 * is tossed.
		 */
		if (control && mp->msg_flags & MSG_COMPAT) {
			if (mtod(control, struct cmsghdr *)->cmsg_level !=
			    SOL_SOCKET ||
			    mtod(control, struct cmsghdr *)->cmsg_type !=
			    SCM_RIGHTS) {
				mp->msg_controllen = 0;
				goto out;
			}
			control->m_len -= sizeof (struct cmsghdr);
			control->m_data += sizeof (struct cmsghdr);
		}
#endif
		len = mp->msg_controllen;
		m = control;
		mp->msg_controllen = 0;
		ctlbuf = (caddr_t) mp->msg_control;

		while (m && len > 0) {
			unsigned int tocopy;

			if (len >= m->m_len) 
				tocopy = m->m_len;
			else {
				mp->msg_flags |= MSG_CTRUNC;
				tocopy = len;
			}
		
			if ((error = copyout((caddr_t)mtod(m, caddr_t),
					ctlbuf, tocopy)))
				goto out;

			ctlbuf += tocopy;
			len -= tocopy;
			m = m->m_next;
		}
		mp->msg_controllen = ctlbuf - mp->msg_control;
	}
out:
	if (fromsa)
		FREE(fromsa, M_SONAME);
	if (control)
		m_freem(control);
	OSKIT_FREEBSD_DESTROY_CURPROC(p);
	return (error);
}

int 
#if FLASK
bsdnet_recvfrom_secure(struct socket *so, void *buf, size_t len, oskit_u32_t flags, 
       struct sockaddr *from, oskit_size_t *fromlen, oskit_size_t *retval,
	       security_id_t *out_sso_sid, security_id_t *out_msg_sid)
#else
bsdnet_recvfrom(struct socket *so, void *buf, size_t len, oskit_u32_t flags, 
        struct sockaddr *from, oskit_size_t *fromlen, oskit_size_t *retval)
#endif
{
	struct msghdr msg;
	struct iovec aiov;

	msg.msg_namelen = fromlen ? *fromlen : 0;
	msg.msg_name = (caddr_t)from;
	msg.msg_iov = &aiov;
	msg.msg_iovlen = 1;
	aiov.iov_base = buf;
	aiov.iov_len = len;
	msg.msg_control = 0;
	msg.msg_flags = flags;
#if FLASK
	return (bsdnet_recvit_secure(so, &msg, (caddr_t)fromlen, retval,
				     out_sso_sid, out_msg_sid));
#else
	return (bsdnet_recvit(so, &msg, (caddr_t)fromlen, retval));
#endif
}

#if FLASK
int
bsdnet_recvfrom(struct socket *so, void *buf, size_t len, oskit_u32_t flags, 
        struct sockaddr *from, oskit_size_t *fromlen, oskit_size_t *retval)
{
    security_id_t sso_sid, msg_sid;
    return bsdnet_recvfrom_secure(so,buf,len,flags,from,fromlen,retval,
				  &sso_sid, &msg_sid);
}
#endif

