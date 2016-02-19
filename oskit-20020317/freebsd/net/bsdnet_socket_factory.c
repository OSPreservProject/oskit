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
 * The FreeBSD networking stack's implementation of the `oskit_socket_factory'
 * COM interface.
 */

#include <oskit/net/socket.h>
#if FLASK
#  include <oskit/comsid.h>
#endif
#include <oskit/dev/freebsd.h>	/* declares xlate_errno */

#include <sys/param.h>
#include <sys/socketvar.h>

#include "bsdnet_socket_factory.h"
#include "sockimpl.h"
#include "glue.h"

/*****************************************************************************/

#if FLASK
#include <sys/socket.h>
#include <netinet/in.h>
#include "netflask.h"

typedef struct 
{
    oskit_u32_t domain;
    oskit_u32_t type;
    oskit_u32_t protocol;
    oskit_security_class_t sclass;
} so_sclass_t;

static so_sclass_t so_sclass[] = 
{
{ PF_INET, SOCK_STREAM, 0, SECCLASS_IN_STREAM_SOCK },
{ PF_INET, SOCK_DGRAM, 0, SECCLASS_IN_DGRAM_SOCK },
{ PF_INET, SOCK_RAW, IPPROTO_ICMP, SECCLASS_IN_ICMP_SOCK },
{ PF_INET, SOCK_RAW, IPPROTO_IGMP, SECCLASS_IN_IGMP_SOCK },
{ PF_INET, SOCK_RAW, IPPROTO_RSVP, SECCLASS_IN_RSVP_SOCK },
{ PF_INET, SOCK_RAW, IPPROTO_IPIP, SECCLASS_IN_IPIP_SOCK },
{ PF_INET, SOCK_RAW, 0, SECCLASS_IN_RAW_SOCK },
{ PF_ROUTE, SOCK_RAW, 0, SECCLASS_RTSOCK },
};

#define NSOSCLASS (sizeof so_sclass)/sizeof(so_sclass_t)
#endif

oskit_error_t
bsdnet_socket_factory_create_helper(
	oskit_u32_t domain,
	oskit_u32_t type,
	oskit_u32_t protocol, 
#if FLASK
	oskit_security_id_t sid,
#endif
	oskit_socket_t **aso)
{
	oskit_error_t   rc;
        oskit_sockimpl_t *b;
	struct socket  *so;
	struct proc     p;
#if FLASK
	int index;
	oskit_security_id_t csid;
	
	CSID(&csid);

	for (index = 0; index < NSOSCLASS; index++)
	{
	    if (domain == so_sclass[index].domain &&
		type == so_sclass[index].type &&
		(protocol == so_sclass[index].protocol ||
		 so_sclass[index].protocol == 0))
		    break;
	}
	if (index >= NSOSCLASS)
	{
	    osenv_panic("no socket class for (d %d, t %d, p %d)\n",
			domain, type, protocol);
	}

	COMMONSOSIDCHECK(csid, 
			 so_sclass[index].sclass, (sid ? sid : csid),
			 CREATE, &rc);
	if (rc)
		return oskit_freebsd_xlate_errno(rc);
#endif
	OSKIT_FREEBSD_CREATE_CURPROC(p);
        rc = socreate(domain, &so, type, protocol, &p);
	OSKIT_FREEBSD_DESTROY_CURPROC(p);
	if (rc)
		return oskit_freebsd_xlate_errno(rc);
#if FLASK
	so->so_sclass = so_sclass[index].sclass;
	so->so_sid = sid ? sid : csid;
	so->so_newconn_sid = so->so_sid;
	so->so_peer_sid = SECSID_NULL;
	osenv_log(OSENV_LOG_DEBUG, 
		"socket_factory_create():  new socket (sclass %d, sid %d)\n", 
		  so->so_sclass,
		  so->so_sid);
#endif

	b = bsdnet_create_sockimpl(so);
	*aso = &b->ioi;
        return 0;    
}

/*
 * function to create a socket - analog to socket(2)
 */
OSKIT_COMDECL
bsdnet_socket_factory_create(
	oskit_socket_factory_t *factory,
	oskit_u32_t domain,
	oskit_u32_t type,
	oskit_u32_t protocol,
	oskit_socket_t **aso)
{
        return bsdnet_socket_factory_create_helper(domain, type, protocol,
#if FLASK
						   SECSID_NULL,
#endif
						   aso);
}

/*
 * function to create a pair of connected sockets - analog to socketpair(2)
 */
OSKIT_COMDECL
bsdnet_socket_factory_create_pair(
	oskit_socket_factory_t *factory,
	oskit_u32_t domain,
	oskit_u32_t type,
	oskit_u32_t protocol,
	oskit_socket_t **aso1,
	oskit_socket_t **aso2)
{
	return OSKIT_E_NOTIMPL;	/* XXX */
}

/*****************************************************************************/

/* End of file. */

