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
 * Definition of the COM-based ICMP MIB interface.
 */
#ifndef _OSKIT_MIB_ICMP_H_
#define _OSKIT_MIB_ICMP_H_

#include <oskit/com.h>
#include <oskit/mib.h>

/*
 * Struct communicated by `get_icmpstats'.
 */
typedef struct oskit_mib_slotset_icmpstats oskit_mib_slotset_icmpstats_t;
struct oskit_mib_slotset_icmpstats {
	/*
	 * These slots are pretty much verbatim from the SNMPv2 MIB-II `icmp'
	 * group.  For each slot, the SNMP type is shown.  See IETF RFC 2011.
	 */
	
	/* Counter32       */ oskit_mib_u32_t	icmp_in_msgs;
	/* Counter32       */ oskit_mib_u32_t	icmp_in_errors;
	/* Counter32       */ oskit_mib_u32_t	icmp_in_dest_unreachs;
	/* Counter32       */ oskit_mib_u32_t	icmp_in_time_excds;
	/* Counter32       */ oskit_mib_u32_t	icmp_in_parm_probs;
	/* Counter32       */ oskit_mib_u32_t	icmp_in_src_quenchs;
	/* Counter32       */ oskit_mib_u32_t	icmp_in_redirects;
	/* Counter32       */ oskit_mib_u32_t	icmp_in_echos;
	/* Counter32       */ oskit_mib_u32_t	icmp_in_echo_reps;
	/* Counter32       */ oskit_mib_u32_t	icmp_in_timestamps;
	/* Counter32       */ oskit_mib_u32_t	icmp_in_timestamp_reps;
	/* Counter32       */ oskit_mib_u32_t	icmp_in_addr_masks;
	/* Counter32       */ oskit_mib_u32_t	icmp_in_addr_mask_reps;
	
	/* Counter32       */ oskit_mib_u32_t	icmp_out_msgs;
	/* Counter32       */ oskit_mib_u32_t	icmp_out_errors;
	/* Counter32       */ oskit_mib_u32_t	icmp_out_dest_unreachs;
	/* Counter32       */ oskit_mib_u32_t	icmp_out_time_excds;
	/* Counter32       */ oskit_mib_u32_t	icmp_out_parm_probs;
	/* Counter32       */ oskit_mib_u32_t	icmp_out_src_quenchs;
	/* Counter32       */ oskit_mib_u32_t	icmp_out_redirects;
	/* Counter32       */ oskit_mib_u32_t	icmp_out_echos;
	/* Counter32       */ oskit_mib_u32_t	icmp_out_echo_reps;
	/* Counter32       */ oskit_mib_u32_t	icmp_out_timestamps;
	/* Counter32       */ oskit_mib_u32_t	icmp_out_timestamp_reps;
	/* Counter32       */ oskit_mib_u32_t	icmp_out_addr_masks;
	/* Counter32       */ oskit_mib_u32_t	icmp_out_addr_mask_reps;
	
	/*********************************************************************/
	
	/*
	 * The following slots correspond to `SYSCTL'ed data in the FreeBSD 3.x
	 * networking stack that aren't made available through the slots above.
	 * Possibilities for future expansion.
	 */
	
	/* In `freebsd/3.x/src/sys/netinet/ip_icmp.c':			*/
	/* oskit_mib_bool_t			icmp_mask_repl;		*/
	/* oskit_mib_s32_t			icmp_lim;		*/
	/* oskit_mib_bool_t			icmp_bmcast_echo;	*/
	
	/*
	 * In `freebsd/3.x/src/sys/netinet/icmp_var.h': fields of `icmpstat'
	 * that aren't available, or aren't available individually.
	 */
	/* oskit_mib_u32_t			icps_error;		*/
	/* oskit_mib_u32_t			icps_oldshort;		*/
	/* oskit_mib_u32_t			icps_oldicmp;		*/
	/* oskit_mib_u32_t			icps_badcode;		*/
	/* oskit_mib_u32_t			icps_tooshort;		*/
	/* oskit_mib_u32_t			icps_checksum;		*/
	/* oskit_mib_u32_t			icps_badlen;		*/
	/* oskit_mib_u32_t			icps_reflect;		*/
	/* oskit_mib_u32_t			icps_bmcastecho;	*/
	/* oskit_mib_u32_t			icps_bmcasttstamp;	*/
	/* ...and a few of the members of	icps_inhist[];		*/
	/* ...and a few of the memebrs of	icps_outhist[];		*/
};

/*
 * ICMP MIB interface.
 * IID 4aa7e004-7c74-11cf-b500-08000953adc2
 */
struct oskit_mib_icmp {
	struct oskit_mib_icmp_ops *ops;
};
typedef struct oskit_mib_icmp oskit_mib_icmp_t;

struct oskit_mib_icmp_ops {
	/*
	 * COM-specified IUnknown interface operations.
	 */
	OSKIT_COMDECL_IUNKNOWN(oskit_mib_icmp_t)

	/*
	 * Operations specific to the ICMP monitor interface.
	 *
	 * In the SNMPv2 MIB-II `icmp' group, all slots are read-only.
	 */
	OSKIT_MIB_SLOTSET_OPS_RO	(oskit_mib_icmp_t,
					 oskit_mib_slotset_icmpstats_t,
					 icmpstats)
};

#define oskit_mib_icmp_query(icmp, iid, out_ihandle) \
	((icmp)->ops->query((oskit_mib_icmp_t *)(icmp), (iid), (out_ihandle)))
#define oskit_mib_icmp_addref(icmp) \
	((icmp)->ops->addref((oskit_mib_icmp_t *)(icmp)))
#define oskit_mib_icmp_release(icmp) \
	((icmp)->ops->release((oskit_mib_icmp_t *)(icmp)))

#define oskit_mib_icmp_get_icmpstats(icmp, icmpstats) \
	((icmp)->ops->get_icmpstats((oskit_mib_icmp_t *)(icmp), (icmpstats)))

/* GUID for oskit_mib_icmp interface */
extern const struct oskit_guid oskit_mib_icmp_iid;
#define OSKIT_MIB_ICMP_IID OSKIT_GUID(0x4aa7e004, 0x7c74, 0x11cf, \
				0xb5, 0x00, 0x08, 0x00, 0x09, 0x53, 0xad, 0xc2)

#endif /* _OSKIT_MIB_ICMP_H_ */

/* End of file. */

