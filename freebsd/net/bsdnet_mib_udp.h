/*
 * Copyright (c) 2001 University of Utah and the Flux Group.
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
 * Declarations for the FreeBSD networking stack's implementation of the
 * `oskit_mib_udp' COM interface.
 */
#ifndef _OSKIT_FREEBSD_NET_BSDNET_MIB_UDP_H
#define _OSKIT_FREEBSD_NET_BSDNET_MIB_UDP_H

#include <oskit/mib/udp.h>

OSKIT_COMDECL
bsdnet_mib_udp_get_udpstats(
	oskit_mib_udp_t *				intf,
	/* OUT */ oskit_mib_slotset_udpstats_t *	ss);

OSKIT_COMDECL
bsdnet_mib_udp_get_udpentry_table(
	oskit_mib_udp_t *				intf,
	oskit_u32_t					start_row,
	oskit_u32_t					want_rows,
	oskit_mib_slotset_udpentry_t *			matching,
	/* OUT */ oskit_mib_slotset_udpentry_t *	table,
	/* OUT */ oskit_u32_t *				out_rows,
	/* OUT */ oskit_bool_t *			more_rows);

#endif /* _OSKIT_FREEBSD_NET_BSDNET_MIB_UDP_H */

/* End of file. */

