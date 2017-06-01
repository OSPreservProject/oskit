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
 * bsdnet_sendto() - derived from code in /usr/src/sys/kern/uipc_syscalls.c
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

static int     
#if FLASK
bsdnet_sendit_secure(so, mp, flags, retsize, dso_sid, msg_sid)   
        struct socket *so;
        register struct msghdr *mp;
        int flags, *retsize;
        security_id_t dso_sid, msg_sid;
#else
bsdnet_sendit(so, mp, flags, retsize)   
        struct socket *so;
        register struct msghdr *mp;
        int flags, *retsize;
#endif
{               
        struct uio auio;
        register struct iovec *iov;
        register int i;
	struct proc p;
	struct sockaddr *to = 0;
        struct mbuf *control;
        int len, error;

	OSKIT_FREEBSD_CREATE_CURPROC(p);
        auio.uio_iov = mp->msg_iov;
        auio.uio_iovcnt = mp->msg_iovlen;
        auio.uio_segflg = UIO_USERSPACE;
        auio.uio_rw = UIO_WRITE;
        auio.uio_procp = &p;
        auio.uio_offset = 0;                    /* XXX (BSD) */
        auio.uio_resid = 0;
        iov = mp->msg_iov;
        for (i = 0; i < mp->msg_iovlen; i++, iov++) {
                if ((auio.uio_resid += iov->iov_len) < 0) {
                        error = EINVAL;
			goto bad;
		}
        }
        if (mp->msg_name) {
		error = getsockaddr(&to, mp->msg_name, mp->msg_namelen);
                if (error)
                        goto bad;
        } else
                to = 0;
        if (mp->msg_control) {
                if (mp->msg_controllen < sizeof(struct cmsghdr)
		    /* the next line is COMPAT_OLDSOCK */
                    && mp->msg_flags != MSG_COMPAT	
                ) {
                        error = EINVAL;
                        goto bad;
                }
                error = sockargs(&control, mp->msg_control,
                    mp->msg_controllen, MT_CONTROL);
                if (error)
                        goto bad;
#ifdef COMPAT_OLDSOCK
                if (mp->msg_flags == MSG_COMPAT) {
                        register struct cmsghdr *cm;

                        M_PREPEND(control, sizeof(*cm), M_WAIT);
                        if (control == 0) {
                                error = ENOBUFS;
                                goto bad;
                        } else {
                                cm = mtod(control, struct cmsghdr *);
                                cm->cmsg_len = control->m_len;
                                cm->cmsg_level = SOL_SOCKET;
                                cm->cmsg_type = SCM_RIGHTS;
                        }
                }
#endif
        } else
                control = 0;

        len = auio.uio_resid;
#if FLASK
        error = so->so_proto->pr_usrreqs->pru_sosend_secure(so, to, &auio, (struct mbuf *)0, control, flags, &p,
			      dso_sid, msg_sid);
#else
        error = so->so_proto->pr_usrreqs->pru_sosend(so, to, &auio, (struct mbuf *)0, control, flags, &p);
#endif
        if (error) {
                if (auio.uio_resid != len && (error == ERESTART ||
                    error == EINTR || error == EWOULDBLOCK))
                        error = 0;
                if (error == EPIPE)
                        psignal(curproc, SIGPIPE);
        }
        if (error == 0)
                *retsize = len - auio.uio_resid;
bad:
        if (to)
		FREE(to, M_SONAME);
	OSKIT_FREEBSD_DESTROY_CURPROC(p);
        return (error);
}

int
sockargs(mp, buf, buflen, type)
        struct mbuf **mp;
        caddr_t buf;
        int buflen, type;
{
        register struct sockaddr *sa;
        register struct mbuf *m;
        int error;

        if ((u_int)buflen > MLEN) {
#ifdef COMPAT_OLDSOCK
                if (type == MT_SONAME && (u_int)buflen <= 112)
                        buflen = MLEN;          /* unix domain compat. hack */
                else
#endif
                return (EINVAL);
        }
        m = m_get(M_WAIT, type);
        if (m == NULL)
                return (ENOBUFS);
        m->m_len = buflen;
        error = copyin(buf, mtod(m, caddr_t), (u_int)buflen);
        if (error)
                (void) m_free(m);
        else {
                *mp = m;
                if (type == MT_SONAME) {
                        sa = mtod(m, struct sockaddr *);

#if defined(COMPAT_OLDSOCK) && BYTE_ORDER != BIG_ENDIAN
                        if (sa->sa_family == 0 && sa->sa_len < AF_MAX)
                                sa->sa_family = sa->sa_len;
#endif
                        sa->sa_len = buflen;
                }
        }
        return (error);
}


int 
#if FLASK
bsdnet_sendto_secure(struct socket *so, const void *buf, size_t len, oskit_u32_t flags,
        const struct sockaddr *to, oskit_size_t tolen, oskit_size_t *retval,
		     security_id_t dso_sid, security_id_t msg_sid)
#else
bsdnet_sendto(struct socket *so, const void *buf, size_t len, oskit_u32_t flags,
        const struct sockaddr *to, oskit_size_t tolen, oskit_size_t *retval)
#endif
{
        struct msghdr msg;
        struct iovec aiov; 
 
        msg.msg_name = (caddr_t)to;
        msg.msg_namelen = tolen;
        msg.msg_iov = &aiov;
        msg.msg_iovlen = 1;
        msg.msg_control = 0;
        msg.msg_flags = 0;
        aiov.iov_base = (caddr_t)buf;
        aiov.iov_len = len;
#if FLASK
        return (bsdnet_sendit_secure(so, &msg, flags, retval, dso_sid, msg_sid));
#else
        return (bsdnet_sendit(so, &msg, flags, retval));
#endif
}

#if FLASK
int
bsdnet_sendto(struct socket *so, const void *buf, size_t len, oskit_u32_t flags,
        const struct sockaddr *to, oskit_size_t tolen, oskit_size_t *retval)
{
    return bsdnet_sendto_secure(so,buf,len,flags,to,tolen,retval,SECSID_NULL,
				SECSID_NULL);
}
#endif

