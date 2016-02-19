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
 * bsdnet_read()
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
#include <sys/uio.h>
#include <oskit/net/freebsd.h>
#include <machine/limits.h>

#include "glue.h"

/* 
 * Derived from read in uipc_syscalls.c and soo_read in sys_socket.c
 */
int
bsdnet_read(struct socket *so, void *buf, size_t nbytes, oskit_u32_t *retval)
{
	int 	error;
	struct uio auio;
	struct iovec aiov;
	struct proc p;

	if (nbytes < 0)
		return EINVAL;

	OSKIT_FREEBSD_CREATE_CURPROC(p);

	aiov.iov_base = (caddr_t)buf;
	aiov.iov_len = nbytes;

	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_offset = -1;
	if (nbytes > INT_MAX) 
	{
	      OSKIT_FREEBSD_DESTROY_CURPROC(p);
	      return (EINVAL);
	}
	auio.uio_resid = aiov.iov_len;
	auio.uio_rw = UIO_READ;
	auio.uio_segflg = UIO_USERSPACE;
	auio.uio_procp = &p;

	if ((error = so->so_proto->pr_usrreqs->pru_soreceive(so, (struct sockaddr **)0, &auio, (struct mbuf **)0, 
			(struct mbuf **)0, (int *)0))) 
	{
		if (auio.uio_resid != nbytes && (error == ERESTART || 
			error == EINTR || error == EWOULDBLOCK))
				error = 0;
	}
	if (retval)
		*retval = nbytes - auio.uio_resid;
	OSKIT_FREEBSD_DESTROY_CURPROC(p);
	return error;
}
