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
 * Definition of the COM-based UDP MIB interface.
 */
#ifndef _OSKIT_MIB_UDP_H_
#define _OSKIT_MIB_UDP_H_

#include <oskit/com.h>
#include <oskit/mib.h>

/*
 * Struct communicated by `get_udpstats'.
 */
typedef struct oskit_mib_slotset_udpstats oskit_mib_slotset_udpstats_t;
struct oskit_mib_slotset_udpstats {
	/*
	 * These slots are pretty much verbatim from the SNMPv2 MIB-II `udp'
	 * group, minus the tables.  For each slot, the SNMP type is shown.
	 * See IETF RFC 2013.
	 */
	
	/* Counter32       */ oskit_mib_u32_t	udp_in_datagrams;
	/* Counter32       */ oskit_mib_u32_t	udp_no_ports;
	/* Counter32       */ oskit_mib_u32_t	udp_in_errors;
	
	/* Counter32       */ oskit_mib_u32_t	udp_out_datagrams;
	
	/* SEQUENCE OF UdpEntry			udp_table; */
	
	/*********************************************************************/
	
	/*
	 * The following slots correspond to `SYSCTL'ed data in the FreeBSD 3.x
	 * networking stack that aren't made available through the slots above.
	 * Possibilities for future expansion.
	 */
	
	/* In `freebsd/3.x/src/sys/netinet/udp_usrreq.c':		*/
	/* oskit_mib_bool_t			udpcksum;		*/
	/* oskit_mib_bool_t			log_in_vain;		*/
	/* oskit_mib_u32_t			udp_maxdgram;		*/
	/* oskit_mib_u32_t			udp_recvspace;		*/
	
	/*
	 * In `freebsd/3.x/src/sys/netinet/udp_var.h': fields of `udpstat' that
	 * aren't available, or aren't available individually.
	 * (`udps_ipackets' is actually easy to determine from values above.)
	 */
	/* oskit_mib_u32_t			udps_ipackets;		*/
	/* oskit_mib_u32_t			udps_hdrops;		*/
	/* oskit_mib_u32_t			udps_badsum;		*/
	/* oskit_mib_u32_t			udps_badlen;		*/
	/* oskit_mib_u32_t			udps_noportbcast;	*/
	/* oskit_mib_u32_t			udps_fullsock;		*/
	/* oskit_mib_u32_t			udpps_pcbcachemiss;	*/
	/* oskit_mib_u32_t			udpps_pcbhashmiss;	*/
	/* oskit_mib_u32_t			udps_fastout;		*/
};

/*
 * Struct communicated by `get_udpentry_table'.
 */
typedef struct oskit_mib_slotset_udpentry oskit_mib_slotset_udpentry_t;
struct oskit_mib_slotset_udpentry {
	/*
	 * These slots are pretty much verbatim from the SNMPv2 MIB-II `udp'
	 * group's `udpEntry' sequence.  For each slot, the SNMP type is shown.
	 * See IETF RFC 2013.
	 */
	
	/* IpAddress         */ oskit_mib_mib_in_addr_t	udp_local_address;
	/* INTEGER(0..65535) */ oskit_mib_u16_t		udp_local_port;
};

/*
 * UDP MIB interface.
 * IID 4aa7e006-7c74-11cf-b500-08000953adc2
 */
struct oskit_mib_udp {
	struct oskit_mib_udp_ops *ops;
};
typedef struct oskit_mib_udp oskit_mib_udp_t;

struct oskit_mib_udp_ops {
	/*
	 * COM-specified IUnknown interface operations.
	 */
	OSKIT_COMDECL_IUNKNOWN(oskit_mib_udp_t)

	/*
	 * Operations specific to the UDP monitor interface.
	 *
	 * In the SNMPv2 MIB-II `udp' group, all slots are read-only.
	 */
	OSKIT_MIB_SLOTSET_OPS_RO	(oskit_mib_udp_t,
					 oskit_mib_slotset_udpstats_t,
					 udpstats)
	OSKIT_MIB_SLOTSET_TABLE_OPS_RO	(oskit_mib_udp_t,
					 oskit_mib_slotset_udpentry_t,
					 udpentry)
};

#define oskit_mib_udp_query(udp, iid, out_ihandle) \
	((udp)->ops->query((oskit_mib_udp_t *)(udp), (iid), (out_ihandle)))
#define oskit_mib_udp_addref(udp) \
	((udp)->ops->addref((oskit_mib_udp_t *)(udp)))
#define oskit_mib_udp_release(udp) \
	((udp)->ops->release((oskit_mib_udp_t *)(udp)))

#define oskit_mib_udp_get_udpstats(udp, udpstats) \
	((udp)->ops->get_udpstats((oskit_mib_udp_t *)(udp), (udpstats)))

#define oskit_mib_udp_get_udpentry_table(udp,				 \
					 start_row, want_rows, matching, \
					 table, out_rows, more_rows)	 \
	((udp)->ops->get_udpentry_table((oskit_mib_udp_t *)(udp),	 \
					(start_row),			 \
					(want_rows),			 \
					(matching),			 \
					/* OUT */ (table),		 \
					/* OUT */ (out_rows),		 \
					/* OUT */ (more_rows)))

/* GUID for oskit_mib_udp interface */
extern const struct oskit_guid oskit_mib_udp_iid;
#define OSKIT_MIB_UDP_IID OSKIT_GUID(0x4aa7e006, 0x7c74, 0x11cf, \
				0xb5, 0x00, 0x08, 0x00, 0x09, 0x53, 0xad, 0xc2)

#endif /* _OSKIT_MIB_UDP_H_ */

/* End of file. */

