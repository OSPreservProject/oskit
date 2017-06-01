/*
 * Copyright (c) 1997-2000, 2002 University of Utah and the Flux Group.
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
 * This file simulates the "receive" of a FreeBSD device driver
 * "bsdnet_net_receive" is the callback function used for the push
 * method of the fdev netio instance. There, incoming packets are
 * analyzed and "mbuf"ized before they're passed up to the networking
 * code via ether_input(). 
 *
 * ether_input() will queue the packet on the interface, and might
 * post a software irq for the processing. If it does, we immediately
 * execute the software irq.
 *
 * Note the use of the spl*() and save_cpl/restore_cpl here.
 * We must simulate what the code in /usr/src/sys/i386/isa/vector.s and
 * /usr/src/sys/i386/isa/icu.s does
 *
 * In particular, it does:
 *	movl    _cpl,%eax ; \
 *	pushl   %eax ; \
 *	...
 *	orl     _intr_mask + (irq_num) * 4,%eax ; \
 *	movl    %eax,_cpl ; \
 *	...
 *	call    *_intr_handler + (irq_num) * 4 ; \
 *	...
 *	jmp     _doreti
 *	<...>
 * _doreti:
 *      ...
 *      popl    %eax                    cpl to restore
 *      ...
 *      movl    %eax,_cpl
 *      ...
 *      iret
 *
 * Also, compare to the "send" side of things in bsdnet_driver.c
 */

/*
 * OSKit headers
 */
#include <oskit/dev/dev.h>
#include <oskit/dev/net.h>
#include <oskit/dev/ethernet.h>
#include <oskit/net/freebsd.h>

#include <machine/ipl.h>

/*
 * include BSD headers
 */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <machine/endian.h>
#include <machine/param.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>

#include "bsdnet_net_io.h"

#define VERBOSITY 0

extern volatile unsigned oskit_freebsd_ipending;
extern unsigned oskit_freebsd_idelayed;
volatile unsigned int    netisr;
char * ethtoa(unsigned char p[6]);

/*
 * We don't see the 64bit preamble or 32bit CRC so our
 * ETHERMAX is a little less than the 1526 ethernet frame size.
 */
#define ETHERMAX        (1526-8-4)

oskit_error_t
bsdnet_net_receive(void *data, oskit_bufio_t *b, oskit_size_t pkt_size)
{
        struct mbuf     	*m;
	struct ether_header     *eh;
	unsigned char   	*frame;
	int 			rval = 0, err = 0, length, payload;
        caddr_t         	rb;
	oskit_freebsd_net_ether_if_t *eif = data;
	struct ifnet 		*ifp = eif->ifp;
	unsigned		cpl;
	int			s;

	if (pkt_size > ETHERMAX) {
		osenv_log(OSENV_LOG_WARNING, 
			"%s: Hey Wally, I caught a big one! -- %d bytes\n",
			       eif->info.name, pkt_size);
		return OSKIT_E_DEV_BADPARAM;
	}

	/* we are not associated with an interface yet 
	 * so we can't do anything
	 */
	if (!ifp)
		return 0;

	/* map data in */
	/* buf, dest, offset, count */
	err = oskit_bufio_map(b, (void **)&frame, 0, pkt_size);
	if (err)
		osenv_panic("oskit_bufio_map returned 0x%x\n", err);

	/* only used by ether_input... -> just pass a pointer to our buffer */
	eh = (struct ether_header *)frame;

	{
		int type = ntohs(eh->ether_type);

		/* ignore IEEE 802.3 packets */
		if (type < ETHERMTU)
			return 0;

#if VERBOSITY > 0
		osenv_log(OSENV_LOG_INFO, 
			"RECV: src = %s", ethtoa(eh->ether_shost));
		osenv_log(OSENV_LOG_INFO, " to %s, type = x%x, size = %d\n", 
			ethtoa(eh->ether_dhost), type, pkt_size);
#endif
	}

	/* keep a reference around! */
	oskit_bufio_addref(b);

	/* NOW we're getting BSDish... */

	/* save cpl */
	save_cpl(&cpl);

	/* we set the cpl to what it would be if BSD processed that interrupt */
	osenv_assert(osenv_intr_enabled() != 0);
	s = splimp();

	/* 
 	 * this code is modeled from /usr/src/sys/....../if_ix.c 
	 */
        MGETHDR(m, M_DONTWAIT, MT_DATA);
        if (m == 0) {
                osenv_log(OSENV_LOG_INFO, "MGETHDR:"); 
			/* ZZZ need to add drop counters */
                goto done;
        }
        length = pkt_size;
        payload = length - sizeof(struct ether_header);

        m->m_pkthdr.rcvif = ifp;
        m->m_pkthdr.len = payload;

	/*
	 * here we go, faking an mbuf cluster
	 */
	m->m_ext.ext_buf = frame;
	m->m_ext.ext_size = length;
	m->m_data = rb = frame + sizeof(struct ether_header);
        m->m_len = payload;

	/*
	 * mark this mbuf as referring to an oskit bufio 
	 */
	m->m_ext.ext_bufio = b;
	m->m_flags |= M_EXT;

	/* we set the cpl to what it would be if BSD processed that interrupt */
	splnet();

	/* 
	 * hand packet up as if it came fresh from a device driver 
	 */
	ether_input(ifp, eh, m);
	ifp->if_ipackets++;

done:
	osenv_assert(osenv_intr_enabled() == 0);

	splx(s);
	/* now restore the cpl */
	restore_cpl(cpl);

	return rval;
}

void
bsdnet_net_softnet(void)
{
	int s = splnet();
	int i;

	osenv_assert((oskit_freebsd_ipending & SWI_NET_PENDING) != 0);
	for (i = 0; i < 32; i++) {
		if (netisr & (1 << i)) {
			netisrs[i]();
		}
		netisr &= ~(1 << i);
	}
	oskit_freebsd_ipending &= ~SWI_NET_PENDING;

	splx(s);
}
