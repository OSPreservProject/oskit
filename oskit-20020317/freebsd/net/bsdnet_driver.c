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
 * This file contains the code for the device driver routines that the
 * BSD TCP/IP stack needs. In particular, it contains a driver_ioctl
 * and a driver_start method. Essentially, we "simulate" a FreeBSD
 * compliant device driver on top of OSKit's netio. 
 *
 * This is the "send" side - for the "receive" side see net_receive.c
 */

#include <oskit/dev/dev.h>
#include <oskit/dev/net.h>
#include <oskit/dev/ethernet.h>
#include <oskit/io/netio.h>
#include <oskit/net/freebsd.h>

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
#include "net/if.h"
#include "net/route.h"
#include "netinet/in.h"
#include "netinet/if_ether.h"
#include "netinet/in_var.h"

#include "bsdnet_net_io.h"
#include "mbuf_buf_io.h"

#define VERBOSITY 	0
#define assert(x)	{ if (!(x)) osenv_panic(__FILE__":%d:"#x, __LINE__); }

/* -------------------------------------------------------------------- */

/*
 * in this function, we try to mimic all the actions taken by a driver,
 * say the DEC Tulip driver (see /usr/src/sys/pci/if_de.c:tulip_ioctl)
 *
 * I have now clue if and why the driver has to do that... it seems to 
 * violate any principle of layering
 */
int
bsdnet_driver_ioctl(struct ifnet * ifp, int cmd, char *data)
{
	struct arpcom *arp = (struct arpcom *)ifp;
	struct ifaddr *ifa = (struct ifaddr *)data;
	struct ifreq *ifr = (struct ifreq *) data;
	int s, error = 0;

	s = splimp();

	switch (cmd) {
	case SIOCSIFADDR: {

		/* we're up and running! */
		ifp->if_flags |= IFF_UP | IFF_RUNNING;

		switch(ifa->ifa_addr->sa_family) {
                case AF_INET: {
			arp_ifinit(arp, ifa);
			break; 
                }
		default:
			osenv_log(OSENV_LOG_ERR, 
				"Oops .. don't know what to do now\n");
		}
		break;
	} 

        case SIOCGIFADDR: {
		bcopy((caddr_t) arp->ac_enaddr,
		      (caddr_t) ((struct sockaddr *)&ifr->ifr_data)->sa_data,
		      6);
		break;
        }

	case SIOCSIFFLAGS: {
		osenv_log(OSENV_LOG_WARNING, 
			__FUNCTION__": SIOCSIFFLAGS was called and ignored\n");
		break;
	}

	case SIOCADDMULTI: {
		osenv_log(OSENV_LOG_WARNING, 
			__FUNCTION__": SIOCADDMULTI was called -- beats me\n");
		/* FALLTHROUGH */
	}


	default:
		osenv_log(OSENV_LOG_WARNING, 
			"Don't know what to do with cmd = x%x in %s:%d...\n", 
			cmd, __FILE__, __LINE__);
		error = EINVAL;
	}

	splx(s);      
	return error; 
}


/* 
 * This function is always called after a packet has been enqueued on the
 * interface queue - we're asked to start transmitting packets off the
 * interface queue.
 *
 * Note that we do not set IFF_OACTIVE which would tell ether_output
 * to not call us again.  We can't let that happen since we have no
 * "transmit done" style interrupt to tell us to clear the flag and thus
 * all packet transmission would stop.
 *
 * Note that we do return a value even though BSD if_start functions are
 * defined as void.  This is because there is OSKIT code in ether_output
 * which calls us directly and knows about the return value.
 */
int
bsdnet_driver_start(struct ifnet *ifp)
{
	struct ifqueue * const ifq = &ifp->if_snd;
	struct mbuf *m;
	oskit_netio_t *nio;
	int err = 0;

	/* we could be here too early */
	if (!ifp->nio)
		return -1;

	/* 
	 * get the object used to send 
	 */
	nio = ifp->nio;

	if ((ifp->if_flags & IFF_RUNNING) == 0)
		return -1;

	/*
	 * now send all packets that are queued up
	 */
	while (err == 0) {
		oskit_bufio_t *b;
		oskit_size_t   mlen;

		/* dequeue a packet */
		IF_DEQUEUE(ifq, m);
		/* nothing to dequeue - done */
		if (m == NULL)
		    break;

		b = mbuf_bufio_create_instance(m, &mlen);

#if VERBOSITY > 0
		{
		    struct ether_header *eth = mtod(m, struct ether_header *);
            
		    osenv_log(OSENV_LOG_INFO, 
			"SEND: src = %s ", ethtoa(eth->ether_shost));
		    osenv_log(OSENV_LOG_INFO, 
			" dst = %s ", ethtoa(eth->ether_dhost));
		    osenv_log(OSENV_LOG_INFO, 
			" type = x%x, len = %d\n", ntohs(eth->ether_type),
			    mlen);
		}
#endif

		err = oskit_netio_push(nio, b, mlen);
		oskit_bufio_release(b);
	}

	return err;
}

/* ------------------------ helper routines ------------------------ */
char *
ethtoa(unsigned char p[6])
{
        static char buf[6*3];           /* nn:nn:nn:nn:nn:nnz */
        sprintf(buf, "%02x:%02x:%02x:%02x:%02x:%02x",
                p[0], p[1], p[2], p[3], p[4], p[5]);
        return buf;
}

