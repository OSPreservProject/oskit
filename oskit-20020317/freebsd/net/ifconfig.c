/*
 * Copyright (c) 1997-1999, 2001 University of Utah and the Flux Group.
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
 * setting up an interface -- as in
 * ifconfig de0 inet 155.99.214.164 link2 netmask 255.255.255.0
 */

#include <oskit/dev/dev.h>
#include <oskit/dev/net.h>
#include <oskit/dev/ethernet.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/sockio.h>
#include <string.h>
#include "net/if.h"
#include "net/route.h"
#include "netinet/in.h"
#include "netinet/if_ether.h"
#include "netinet/in_var.h"
#include <arpa/inet.h>

#include <oskit/net/freebsd.h>
#include "bsdnet_net_io.h"
#include "mbuf_buf_io.h"
#include "glue.h"

static MALLOC_DEFINE(M_ARP, "ARP", "oskit ifconfig");


/**
 * see start_if.c on how to obtain a valid oskit_freebsd_net_ether_if_t
 * call it like so:
 *
 * oskit_freebsd_net_ifconfig(eif, "de0", "155.99.214.164", "255.255.255.0")
 */
oskit_error_t
oskit_freebsd_net_ifconfig(oskit_freebsd_net_ether_if_t *eif, 
	char *name, 
	char *ipaddr, 
	char *netmask) 
{
	struct arpcom *arp;
	struct in_aliasreq addreq;
	struct sockaddr_in *sin;
	struct socket so;
	struct proc p;
	struct ifnet *ifp;
	int error = 0, l;
	int default_flags = IFF_BROADCAST|IFF_SIMPLEX|IFF_LINK2;
		/* leave IFF_MULTICAST out or we're getting weird ioctl's */

	OSKIT_FREEBSD_CREATE_CURPROC(p);

	/*
	 * this stuff is usually done during the autoprobe
	 */
	arp = (struct arpcom *)malloc(sizeof *arp, M_ARP, 0);
	if (!arp)
		return OSKIT_ENOMEM;

	/* necessary!!! */
	memset(arp, 0, sizeof *arp);

	/* copy ethernet address where ARP needs it */
	memcpy(arp->ac_enaddr, eif->haddr, OSKIT_ETHERDEV_ADDR_SIZE);

        ifp = &arp->ac_if;
	l = strlen(name); 
	ifp->if_unit = name[l-1] - '0'; 
	ifp->if_name = malloc(l, M_ARP, 0); 
	strncpy(ifp->if_name, name, l-1); 
	ifp->if_name[l-1] = 0; 

	ifp->if_flags = default_flags;
	ifp->if_ioctl = bsdnet_driver_ioctl;
	ifp->if_output = ether_output;
	ifp->if_start = bsdnet_driver_start;

	/* have to set the mtu somewhere! */
	ifp->if_mtu = ETHERMTU;

	/* in FreeBSD, this is done in ifinit() which iterates through
	 * the list of found interfaces - in our case, ifinit() is already
	 * through and didn't have any if's in the list 
	 */
	ifp->if_snd.ifq_maxlen = IFQ_MAXLEN;

	osenv_log(OSENV_LOG_DEBUG, 
		">>>Attaching network interface %s\n", name);

	/* attach the interface */
	if_attach(ifp);
	ether_ifattach(ifp);

	/* ------ the actual ifconfig part starts here ------ */

	/*
	 * we're only doing the absolutely necessary minimum here. 
	 *
	 * a better implementation would take /usr/src/sbin/ifconfig/ifconfig.c
	 * and turn it into a function
	 */

	/* set addreq */
	memset(&addreq, 0, sizeof(addreq));	/* to be safe */
	strncpy(addreq.ifra_name, name, sizeof addreq.ifra_name);

	/* set addreq.ifra_addr and addreq.ifra_mask */

	sin = (struct sockaddr_in *)&addreq.ifra_addr;
	inet_aton(ipaddr, &sin->sin_addr);
	sin->sin_len = sizeof(*sin); 
	sin->sin_family = AF_INET;

	/* now set netmask */
	sin = (struct sockaddr_in *)&addreq.ifra_mask;
	sin->sin_len = sizeof(*sin);
	inet_aton(netmask, &sin->sin_addr);

	/* in_getaddr, it doesn't set the sin_family to AF_INET why??? */

	/* 
	 * give the ifnet struct a pointer to us, so that it can send stuff
	 */
        ifp->nio = eif->send_nio;

	/*
	 * give our eif a pointer to the interface 
	 * so that it knows where to queue stuff
	 */
	eif->ifp = ifp;

	/* 
	 * now set the new address and netmask using 
	 * SIOCAIFADDR addreq
	 */
	error = in_control(&so, SIOCAIFADDR, (caddr_t) &addreq, ifp, &p);
	if (error) {
		osenv_log(OSENV_LOG_DEBUG, 
			  ">>>Oh No! Network attach failed for %s\n", name);
	}
	
	OSKIT_FREEBSD_DESTROY_CURPROC(p);
	return error;
}


#if FLASK
oskit_error_t
oskit_freebsd_net_ifconfig_secure(oskit_freebsd_net_ether_if_t *eif, 
				  char *name, 
				  char *ipaddr, 
				  char *netmask,
				  security_id_t if_sid)
{
    oskit_error_t rc;
    
    rc = oskit_freebsd_net_ifconfig(eif,name,ipaddr,netmask);
    if (rc)
	    return rc;
    eif->ifp->if_sid = if_sid;
    osenv_log(OSENV_LOG_DEBUG, 
	      ">>>binding SID %d to %s\n",if_sid, name);
    return 0;
}
#endif

