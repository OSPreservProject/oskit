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
 * `oskit_mib_ip' COM interface.
 */
#ifndef _OSKIT_FREEBSD_NET_BSDNET_MIB_IP_H
#define _OSKIT_FREEBSD_NET_BSDNET_MIB_IP_H

#include <oskit/mib/ip.h>

OSKIT_COMDECL
bsdnet_mib_ip_get_ipstats(
	oskit_mib_ip_t *			intf,
	/* OUT */ oskit_mib_slotset_ipstats_t *	ss);

OSKIT_COMDECL
bsdnet_mib_ip_get_ip_forwarding(
	oskit_mib_ip_t *		intf,
	/* OUT */ oskit_mib_bool_t *	val);

OSKIT_COMDECL
bsdnet_mib_ip_set_ip_forwarding(
	oskit_mib_ip_t *		intf,
	oskit_bool_t *			val);

OSKIT_COMDECL
bsdnet_mib_ip_get_ip_default_ttl(
	oskit_mib_ip_t *		intf,
	/* OUT */ oskit_mib_u8_t *	val);

OSKIT_COMDECL
bsdnet_mib_ip_set_ip_default_ttl(
	oskit_mib_ip_t *		intf,
	oskit_u8_t *			val);

OSKIT_COMDECL
bsdnet_mib_ip_get_ipaddr_table(
	oskit_mib_ip_t *				intf,
	oskit_u32_t					start_row,
	oskit_u32_t					want_rows,
	oskit_mib_slotset_ipaddr_t *			matching,
	/* OUT */ oskit_mib_slotset_ipaddr_t *		table,
	/* OUT */ oskit_u32_t *				out_rows,
	/* OUT */ oskit_bool_t *			more_rows);

OSKIT_COMDECL
bsdnet_mib_ip_get_ipnettomedia_table(
	oskit_mib_ip_t *				intf,
	oskit_u32_t					start_row,
	oskit_u32_t					want_rows,
	oskit_mib_slotset_ipnettomedia_t *		matching,
	/* OUT */ oskit_mib_slotset_ipnettomedia_t *	table,
	/* OUT */ oskit_u32_t *				out_rows,
	/* OUT */ oskit_bool_t *			more_rows);

OSKIT_COMDECL
bsdnet_mib_ip_get_ipforward_table(
	oskit_mib_ip_t *				intf,
	oskit_u32_t					start_row,
	oskit_u32_t					want_rows,
	oskit_mib_slotset_ipforward_t *			matching,
	/* OUT */ oskit_mib_slotset_ipforward_t *	table,
	/* OUT */ oskit_u32_t *				out_rows,
	/* OUT */ oskit_bool_t *			more_rows);

OSKIT_COMDECL
bsdnet_mib_ip_set_ipforward_table(
	oskit_mib_ip_t *				intf,
	oskit_mib_slotset_ipforward_t *			matching,
	oskit_bool_t					all_matches,
	oskit_mib_slotset_ipforward_t *			ss);

#endif /* _OSKIT_FREEBSD_NET_BSDNET_MIB_IP_H */

/* End of file. */

