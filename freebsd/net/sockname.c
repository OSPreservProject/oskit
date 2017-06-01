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
 * bsdnet_getsockname() - derived from code in /usr/src/sys/kern/uipc_syscalls.c
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
#include <sys/protosw.h>
#include <oskit/net/freebsd.h>

#include "glue.h"

int
bsdnet_getpeername(struct socket *so, struct sockaddr *asa, oskit_size_t *alen)
{
	struct sockaddr *sa;
	struct proc p;
	int len, error;

	if ((so->so_state & (SS_ISCONNECTED|SS_ISCONFIRMING)) == 0)
		return (ENOTCONN);

	OSKIT_FREEBSD_CREATE_CURPROC(p);

	/* XXX copyin! */
	len = *alen;

	sa = 0;
	error = (*so->so_proto->pr_usrreqs->pru_peeraddr)(so, &sa);
	if (error)
		goto bad;
	if (sa == 0) {
		len = 0;
		goto gotnothing;
	}
#ifndef MIN
#define MIN(a,b) ((a)>(b)?(b):(a))
#endif
	len = MIN(len, sa->sa_len);
#ifdef COMPAT_OLDSOCK
	if (compat)
		((struct osockaddr *)sa)->sa_family =
		    sa->sa_family;
#endif
	error = copyout(sa, (caddr_t)asa, (u_int)len);
	if (error)
		goto bad;
gotnothing:
	*alen = len;
bad:
	if (sa) FREE(sa, M_SONAME);
	OSKIT_FREEBSD_DESTROY_CURPROC(p);
	return (error);
}

int
bsdnet_getsockname(struct socket *so, struct sockaddr *asa, oskit_size_t *alen)
{
        struct sockaddr *sa;
	struct proc p;
	int len, error;

	OSKIT_FREEBSD_CREATE_CURPROC(p);

	/* XXX copyin! */
	len = *alen;

	sa = 0;
	error = (*so->so_proto->pr_usrreqs->pru_sockaddr)(so, &sa);
	if (error)
		goto bad;
	if (sa == 0) {
		len = 0;
		goto gotnothing;
	}
	len = MIN(len, sa->sa_len);
#ifdef COMPAT_OLDSOCK
	if (compat)
		((struct osockaddr *)sa)->sa_family = sa->sa_family;
#endif
	error = copyout(sa, (caddr_t)asa, (u_int)len);
	if (error == 0)
gotnothing:	
	        *alen = len;
bad:
	if (sa)
		FREE(sa, M_SONAME);
	OSKIT_FREEBSD_DESTROY_CURPROC(p);
	return (error);
}

