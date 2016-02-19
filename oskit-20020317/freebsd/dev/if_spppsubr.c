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
 * This set of stubs is just a kludge
 * to get around the fact that the Cronyx driver
 * appears to depend intimately on PPP support being enabled,
 * and won't link without it.
 */

#include <sys/param.h>
#include <sys/systm.h>

#include <net/if.h>
#include <net/if_sppp.h>

void sppp_input (struct ifnet *ifp, struct mbuf *m)
{
	panic("sppp_input");
}

int sppp_output (struct ifnet *ifp, struct mbuf *m, struct sockaddr *dst, struct rtentry *rt)
{
	panic("sppp_output");
}

void sppp_attach (struct ifnet *ifp)
{
	panic("sppp_attach");
}

void sppp_detach (struct ifnet *ifp)
{
	panic("sppp_detach");
}

void sppp_flush (struct ifnet *ifp)
{
	panic("sppp_flush");
}

int sppp_isempty (struct ifnet *ifp)
{
	panic("sppp_isempty");
}

struct mbuf *sppp_dequeue (struct ifnet *ifp)
{
	panic("sppp_dequeue");
}

int sppp_ioctl (struct ifnet *ifp, int cmd, void *data)
{
	panic("sppp_ioctl");
}

