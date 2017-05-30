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
 * The FreeBSD networking stack's implementation of the `oskit_mib_icmp' COM
 * interface.
 */

#include <oskit/mib.h>
#include <oskit/mib/icmp.h>

#include "bsdnet_mib_icmp.h"

#include <sys/param.h>
#include <sys/socket.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/icmp_var.h>

/*
 * The following `SYSCTL' data items are defined in the OSKit file
 * `freebsd/3.x/src/sys/netinet/ip_icmp.c'.
 */
extern struct icmpstat		icmpstat;
/* extern int			icmpmaskrepl; */
/* extern int			icmplim; */
/* extern int			icmpbmcastecho; */

/*****************************************************************************/

OSKIT_COMDECL
bsdnet_mib_icmp_get_icmpstats(
	oskit_mib_icmp_t *				intf,
	/* OUT */ oskit_mib_slotset_icmpstats_t *	ss)
{
	oskit_u32_t icmp_in_msgs;
	oskit_u32_t icmp_in_errors
		= (icmpstat.icps_badcode + icmpstat.icps_tooshort
		   + icmpstat.icps_checksum + icmpstat.icps_badlen);
	oskit_u32_t icmp_out_msgs;
	oskit_u32_t icmp_out_errors
		= (icmpstat.icps_oldshort + icmpstat.icps_oldicmp);
	
	unsigned int i;
	
	/*****/
	
	/* Count all of the received ICMP messages. */
	icmp_in_msgs = icmp_in_errors;
	for (i = 0; i <= ICMP_MAXTYPE; ++i)
		icmp_in_msgs += icmpstat.icps_inhist[i];
	/* Count all of the sent ICMP messages. */
	icmp_out_msgs = icmp_out_errors;
	for (i = 0; i <= ICMP_MAXTYPE; ++i)
		icmp_out_msgs += icmpstat.icps_outhist[i];
	
	oskit_mib_u32_set(&(ss->icmp_in_msgs),
			  icmp_in_msgs);
	oskit_mib_u32_set(&(ss->icmp_in_errors),
			  icmp_in_errors);
	oskit_mib_u32_set(&(ss->icmp_in_dest_unreachs),
			  icmpstat.icps_inhist[ICMP_UNREACH]);
	oskit_mib_u32_set(&(ss->icmp_in_time_excds),
			  icmpstat.icps_inhist[ICMP_TIMXCEED]);
	oskit_mib_u32_set(&(ss->icmp_in_parm_probs),
			  icmpstat.icps_inhist[ICMP_PARAMPROB]);
	oskit_mib_u32_set(&(ss->icmp_in_src_quenchs),
			  icmpstat.icps_inhist[ICMP_SOURCEQUENCH]);
	oskit_mib_u32_set(&(ss->icmp_in_redirects),
			  icmpstat.icps_inhist[ICMP_REDIRECT]);
	oskit_mib_u32_set(&(ss->icmp_in_echos),
			  icmpstat.icps_inhist[ICMP_ECHO]);
	oskit_mib_u32_set(&(ss->icmp_in_echo_reps),
			  icmpstat.icps_inhist[ICMP_ECHOREPLY]);
	oskit_mib_u32_set(&(ss->icmp_in_timestamps),
			  icmpstat.icps_inhist[ICMP_TSTAMP]);
	oskit_mib_u32_set(&(ss->icmp_in_timestamp_reps),
			  icmpstat.icps_inhist[ICMP_TSTAMPREPLY]);
	oskit_mib_u32_set(&(ss->icmp_in_addr_masks),
			  icmpstat.icps_inhist[ICMP_MASKREQ]);
	oskit_mib_u32_set(&(ss->icmp_in_addr_mask_reps),
			  icmpstat.icps_inhist[ICMP_MASKREPLY]);
	
	oskit_mib_u32_set(&(ss->icmp_out_msgs),
			  icmp_out_msgs);
	oskit_mib_u32_set(&(ss->icmp_out_errors),
			  icmp_out_errors);
	oskit_mib_u32_set(&(ss->icmp_out_dest_unreachs),
			  icmpstat.icps_outhist[ICMP_UNREACH]);
	oskit_mib_u32_set(&(ss->icmp_out_time_excds),
			  icmpstat.icps_outhist[ICMP_TIMXCEED]);
	oskit_mib_u32_set(&(ss->icmp_out_parm_probs),
			  icmpstat.icps_outhist[ICMP_PARAMPROB]);
	oskit_mib_u32_set(&(ss->icmp_out_src_quenchs),
			  icmpstat.icps_outhist[ICMP_SOURCEQUENCH]);
	oskit_mib_u32_set(&(ss->icmp_out_redirects),
			  icmpstat.icps_outhist[ICMP_REDIRECT]);
	oskit_mib_u32_set(&(ss->icmp_out_echos),
			  icmpstat.icps_outhist[ICMP_ECHO]);
	oskit_mib_u32_set(&(ss->icmp_out_echo_reps),
			  icmpstat.icps_outhist[ICMP_ECHOREPLY]);
	oskit_mib_u32_set(&(ss->icmp_out_timestamps),
			  icmpstat.icps_outhist[ICMP_TSTAMP]);
	oskit_mib_u32_set(&(ss->icmp_out_timestamp_reps),
			  icmpstat.icps_outhist[ICMP_TSTAMPREPLY]);
	oskit_mib_u32_set(&(ss->icmp_out_addr_masks),
			  icmpstat.icps_outhist[ICMP_MASKREQ]);
	oskit_mib_u32_set(&(ss->icmp_out_addr_mask_reps),
			  icmpstat.icps_outhist[ICMP_MASKREPLY]);
	
	return OSKIT_S_OK;
}

/*****************************************************************************/

/* End of file. */

