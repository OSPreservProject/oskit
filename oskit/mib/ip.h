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
 * Definition of the COM-based IP MIB interface.
 */
#ifndef _OSKIT_MIB_IP_H_
#define _OSKIT_MIB_IP_H_

#include <oskit/com.h>
#include <oskit/mib.h>

/*
 * Struct communicated by `get_ipstats'.
 */
typedef struct oskit_mib_slotset_ipstats oskit_mib_slotset_ipstats_t;
struct oskit_mib_slotset_ipstats {
	/*
	 * These slots are pretty much verbatim from the SNMPv2 MIB-II `ip'
	 * group, minus the tables.  For each slot, the SNMP type is shown.
	 * See IETF RFC 2011.
	 */
	
	/* INTEGER(1..2)   */ oskit_mib_bool_t	ip_forwarding;
	/* INTEGER(1..255) */ oskit_mib_u8_t	ip_default_ttl;
	
	/* Counter32       */ oskit_mib_u32_t	ip_in_receives;
	/* Counter32       */ oskit_mib_u32_t	ip_in_hdr_errors;
	/* Counter32       */ oskit_mib_u32_t	ip_in_addr_errors;
	/* Counter32       */ oskit_mib_u32_t	ip_forw_datagrams;
	/* Counter32       */ oskit_mib_u32_t	ip_in_unknown_protos;
	/* Counter32       */ oskit_mib_u32_t	ip_in_discards;
	/* Counter32       */ oskit_mib_u32_t	ip_in_delivers;
	
	/* Counter32       */ oskit_mib_u32_t	ip_out_requests;
	/* Counter32       */ oskit_mib_u32_t	ip_out_discards;
	/* Counter32       */ oskit_mib_u32_t	ip_out_no_routes;

	/* Integer32       */ oskit_mib_s32_t	ip_reasm_timeout;
	/* Counter32       */ oskit_mib_u32_t	ip_reasm_reqds;
	/* Counter32       */ oskit_mib_u32_t	ip_reasm_oks;
	/* Counter32       */ oskit_mib_u32_t	ip_reasm_fails;
	
	/* Counter32       */ oskit_mib_u32_t	ip_frags_ok;
	/* Counter32       */ oskit_mib_u32_t	ip_frags_fails;
	/* Counter32       */ oskit_mib_u32_t	ip_frags_creates;
	
	/* SEQUENCE OF IpAddrEntry		ip_addr_table; */
	/* (obsolete)				ip_route_table; */
	/* SEQUENCE OF IpNetToMediaEntry	ip_net_to_media_table; */
	
	/* Counter32       */ oskit_mib_u32_t	ip_routing_discards;
	
	/*********************************************************************/
	
	/*
	 * These slots are pretty much verbatim from the SNMP MIB-II
	 * `ipForward' group, minus the tables.  For each slot, the SNMP type
	 * is shown.  See IETF RFC 1354.
	 */
	 
	/* Gauge           */ oskit_mib_u32_t	ip_forward_number;
	/* SEQUENCE OF IpForwardEntry		ip_forward_table; */
	
	/*********************************************************************/
	
	/*
	 * The following slots correspond to `SYSCTL'ed data in the FreeBSD 3.x
	 * networking stack that aren't made available through the slots above.
	 * Possibilities for future expansion.
	 */
	
	/* In `freebsd/3.x/src/sys/netinet/ip_input.c':			*/
	/* oskit_mib_bool_t			ip_send_redirects;	*/
	/* oskit_mib_bool_t			ip_source_route;	*/
	/* oskit_mib_bool_t			ip_accept_source_route;	*/
	/* oskit_mib_s32_t			intrq_max_len;		*/
	/* oskit_mib_s32_t			intrq_drops;		*/
	
	/*
	 * In `freebsd/3.x/src/sys/netinet/ip_var.h': fields of `ipstat' that
	 * aren't available, or aren't available individually.
	 */
	/* oskit_mib_u32_t			ips_badsum;		*/
	/* oskit_mib_u32_t			ips_tooshort;		*/
	/* oskit_mib_u32_t			ips_toosmall;		*/
	/* oskit_mib_u32_t			ips_badhlen;		*/
	/* oskit_mib_u32_t			ips_badlen;		*/
	/* oskit_mib_u32_t			ips_fragtimeout;	*/
	/* oskit_mib_u32_t			ips_fastforward;	*/
	/* oskit_mib_u32_t			ips_redirectsent;	*/
	/* oskit_mib_u32_t			ips_noproto;		*/
	/* oskit_mib_u32_t			ips_badoptions;		*/
	/* oskit_mib_u32_t			ips_badvers;		*/
	/* oskit_mib_u32_t			ips_rawout;		*/
	/* oskit_mib_u32_t			ips_toolong;		*/
	/* oskit_mib_u32_t			ips_notmember;		*/
};

/*
 * Struct communicated by `get_ipaddr_table'.
 */
typedef struct oskit_mib_slotset_ipaddr oskit_mib_slotset_ipaddr_t;
struct oskit_mib_slotset_ipaddr {
	/*
	 * These slots are pretty much verbatim from the SNMPv2 MIB-II `ip'
	 * group's `ipAddrEntry' sequence.  For each slot, the SNMP type is
	 * shown.  See IETF RFC 2011.
	 */
	
	/* IpAddress */
	oskit_mib_mib_in_addr_t			ip_ad_ent_addr;
	
	/* INTEGER(1..2147483647) */
	oskit_mib_u32_t				ip_ad_ent_if_index;
	
	/* IpAddress */
	oskit_mib_mib_in_addr_t			ip_ad_ent_net_mask;
	
	/* INTEGER(0..1) */
	oskit_mib_u32_t				ip_ad_ent_bcast_addr;
	
	/* INTEGER(0..65535) */
	oskit_mib_u16_t				ip_ad_ent_reasm_max_size;
};

/*
 * Struct communicated by `get_ipnettomedia_table'.
 */
typedef struct oskit_mib_slotset_ipnettomedia oskit_mib_slotset_ipnettomedia_t;
struct oskit_mib_slotset_ipnettomedia {
	/*
	 * These slots are pretty much verbatim from the SNMPv2 MIB-II `ip'
	 * group's `ipNetToMediaEntry' sequence.  For each slot, the SNMP type
	 * is shown.  See IETF RFC 2011.
	 */
	
	/* INTEGER(1..2147483647) */
	oskit_mib_u32_t				ip_net_to_media_if_index;
	
	/* PhysAddress */
	oskit_mib_mib_string_t			ip_net_to_media_phys_address;
	
	/* IpAddress */
	oskit_mib_mib_in_addr_t			ip_net_to_media_net_address;
	
	/* INTEGER(1..4) */
	oskit_mib_u32_t				ip_net_to_media_type;
};

/*
 * The values that an `ip_net_to_media_type' slot may have.  Each value
 * corresponds to a member of the `ipNetToMediaType' enumeration defined in
 * IETF RFC 2011.  The encoding is the same as well; i.e., there's no need to
 * translate between the SNMP and OSKIT encodings of the values.
 */
#define OSKIT_MIB_IP_NET_TO_MEDIA_TYPE_OTHER	(1)
#define OSKIT_MIB_IP_NET_TO_MEDIA_TYPE_INVALID	(2)
#define OSKIT_MIB_IP_NET_TO_MEDIA_TYPE_DYNAMIC	(3)
#define OSKIT_MIB_IP_NET_TO_MEDIA_TYPE_STATIC	(4)

/*
 * Struct communicated by `get_ipforward_table'.
 */
typedef struct oskit_mib_slotset_ipforward oskit_mib_slotset_ipforward_t;
struct oskit_mib_slotset_ipforward {
	/*
	 * These slots are pretty much verbatim from the SNMPv2 MIB-II
	 * `ipForward' group's `ipForwardEntry' sequence.  For each slot, the
	 * SNMP type is shown.  See IETF RFC 1354.
	 */
	
	/* IpAddress      */ oskit_mib_mib_in_addr_t	ip_forward_dest;
	/* IpAddress      */ oskit_mib_mib_in_addr_t	ip_forward_mask;
	/* INTEGER        */ oskit_mib_s32_t		ip_forward_policy;
	/* IpAddress      */ oskit_mib_mib_in_addr_t	ip_forward_next_hop;
	/* INTEGER        */ oskit_mib_s32_t		ip_forward_if_index;
	/* INTEGER(1..4)  */ oskit_mib_u32_t		ip_forward_type;
	/* INTEGER(1..15) */ oskit_mib_u32_t		ip_forward_proto;
	/* INTEGER        */ oskit_mib_s32_t		ip_forward_age;
	/* OBJECT IDENTIFIER */				/* ip_forward_info; */
	/* INTEGER        */ oskit_mib_s32_t		ip_forward_next_hop_as;
	/* INTEGER        */ oskit_mib_s32_t		ip_forward_metric1;
	/* INTEGER        */ oskit_mib_s32_t		ip_forward_metric2;
	/* INTEGER        */ oskit_mib_s32_t		ip_forward_metric3;
	/* INTEGER        */ oskit_mib_s32_t		ip_forward_metric4;
	/* INTEGER        */ oskit_mib_s32_t		ip_forward_metric5;
};

/*
 * The values that an `ip_forward_type' slot may have.  Each value corresponds
 * to a member of the `ipForwardType' enumeration defined in IETF RFC 1354.
 * The encoding is the same as well; i.e., there's no need to translate between
 * the SNMP and OSKIT encodings of the values.
 */
#define OSKIT_MIB_IP_FORWARD_TYPE_OTHER		(1)
#define OSKIT_MIB_IP_FORWARD_TYPE_INVALID	(2)
#define OSKIT_MIB_IP_FORWARD_TYPE_LOCAL		(3)
#define OSKIT_MIB_IP_FORWARD_TYPE_REMOTE	(4)

/*
 * The values that an `ip_forward_proto' slot may have.  Each value corresponds
 * to a member of the `ipForwardProto' enumeration defined in IETF RFC 1354.
 * The encoding is the same as well; i.e., there's no need to translate between
 * the SNMP and OSKIT encodings of the values.
 */
#define OSKIT_MIB_IP_FORWARD_PROTO_OTHER	(1)
#define OSKIT_MIB_IP_FORWARD_PROTO_LOCAL	(2)
#define OSKIT_MIB_IP_FORWARD_PROTO_NETMGMT	(3)
#define OSKIT_MIB_IP_FORWARD_PROTO_ICMP		(4)
#define OSKIT_MIB_IP_FORWARD_PROTO_EGP		(5)
#define OSKIT_MIB_IP_FORWARD_PROTO_GGP		(6)
#define OSKIT_MIB_IP_FORWARD_PROTO_HELLO	(7)
#define OSKIT_MIB_IP_FORWARD_PROTO_RIP		(8)
#define OSKIT_MIB_IP_FORWARD_PROTO_IS_IS	(9)
#define OSKIT_MIB_IP_FORWARD_PROTO_ES_IS	(10)
#define OSKIT_MIB_IP_FORWARD_PROTO_CISCO_IGRP	(11)
#define OSKIT_MIB_IP_FORWARD_PROTO_BBN_SPF_IGP	(12)
#define OSKIT_MIB_IP_FORWARD_PROTO_OSPF		(13)
#define OSKIT_MIB_IP_FORWARD_PROTO_BGP		(14)
#define OSKIT_MIB_IP_FORWARD_PROTO_IDPR		(15)

/*
 * IP MIB interface.
 * IID 4aa7e003-7c74-11cf-b500-08000953adc2
 */
struct oskit_mib_ip {
	struct oskit_mib_ip_ops *ops;
};
typedef struct oskit_mib_ip oskit_mib_ip_t;

struct oskit_mib_ip_ops {
	/*
	 * COM-specified IUnknown interface operations.
	 */
	OSKIT_COMDECL_IUNKNOWN(oskit_mib_ip_t)

	/*
	 * Operations specific to the IP monitor interface.
	 *
	 * In the SNMPv2 MIB-II `ip' group, only `ipForwarding' and
	 * `ipDefaultTTL' are read-write.  All of the other slots are
	 * read-only.
	 */
	OSKIT_MIB_SLOTSET_OPS_RO	(oskit_mib_ip_t,
					 oskit_mib_slotset_ipstats_t,
					 ipstats)
	OSKIT_MIB_SLOT_OPS_RW		(oskit_mib_ip_t,
					 bool,
					 ip_forwarding)
	OSKIT_MIB_SLOT_OPS_RW		(oskit_mib_ip_t,
					 u8,
					 ip_default_ttl)
	OSKIT_MIB_SLOTSET_TABLE_OPS_RO	(oskit_mib_ip_t,
					 oskit_mib_slotset_ipaddr_t,
					 ipaddr)
	/* XXX --- Does the following table need to be RW? */
	OSKIT_MIB_SLOTSET_TABLE_OPS_RO	(oskit_mib_ip_t,
					 oskit_mib_slotset_ipnettomedia_t,
					 ipnettomedia)
	OSKIT_MIB_SLOTSET_TABLE_OPS_RW	(oskit_mib_ip_t,
					 oskit_mib_slotset_ipforward_t,
					 ipforward)
};

#define oskit_mib_ip_query(ip, iid, out_ihandle) \
	((ip)->ops->query((oskit_mib_ip_t *)(ip), (iid), (out_ihandle)))
#define oskit_mib_ip_addref(ip) \
	((ip)->ops->addref((oskit_mib_ip_t *)(ip)))
#define oskit_mib_ip_release(ip) \
	((ip)->ops->release((oskit_mib_ip_t *)(ip)))

#define oskit_mib_ip_get_ipstats(ip, ipstats) \
	((ip)->ops->get_ipstats((oskit_mib_ip_t *)(ip), (ipstats)))

#define oskit_mib_ip_get_ip_forwarding(ip, ipfw) \
	((ip)->ops->get_ip_forwarding((oskit_mib_ip_t *)(ip), (ipfw)))
#define oskit_mib_ip_set_ip_forwarding(ip, ipfw) \
	((ip)->ops->set_ip_forwarding((oskit_mib_ip_t *)(ip), (ipfw)))

#define oskit_mib_ip_get_ip_default_ttl(ip, dttl) \
	((ip)->ops->get_ip_default_ttl((oskit_mib_ip_t *)(ip), (dttl)))
#define oskit_mib_ip_set_ip_default_ttl(ip, dttl) \
	((ip)->ops->set_ip_default_ttl((oskit_mib_ip_t *)(ip), (dttl)))

#define oskit_mib_ip_get_ipaddr_table(ip,				\
				      start_row, want_rows, matching,	\
				      table, out_rows, more_rows)	\
	((ip)->ops->get_ipaddr_table((oskit_mib_ip_t *)(ip),		\
				     (start_row),			\
				     (want_rows),			\
				     (matching),			\
				     /* OUT */ (table),			\
				     /* OUT */ (out_rows),		\
				     /* OUT */ (more_rows)))

#define oskit_mib_ip_get_ipnettomedia_table(ip,				    \
					    start_row, want_rows, matching, \
					    table, out_rows, more_rows)	    \
	((ip)->ops->get_ipnettomedia_table((oskit_mib_ip_t *)(ip),	    \
					   (start_row),			    \
					   (want_rows),			    \
					   (matching),			    \
					   /* OUT */ (table),		    \
					   /* OUT */ (out_rows),	    \
					   /* OUT */ (more_rows)))

#define oskit_mib_ip_get_ipforward_table(ip,				 \
					 start_row, want_rows, matching, \
					 table, out_rows, more_rows)	 \
	((ip)->ops->get_ipforward_table((oskit_mib_ip_t *)(ip),		 \
					(start_row),			 \
					(want_rows),			 \
					(matching),			 \
					/* OUT */ (table),		 \
					/* OUT */ (out_rows),		 \
					/* OUT */ (more_rows)))
#define oskit_mib_ip_set_ipforward_table(ip,				\
					 matching, all_matches, vals)	\
	((ip)->ops->get_ipforward_table((oskit_mib_ip_t *)(ip),		\
					(matching),			\
					(all_matches),			\
					(vals)))

/* GUID for oskit_mib_ip interface */
extern const struct oskit_guid oskit_mib_ip_iid;
#define OSKIT_MIB_IP_IID OSKIT_GUID(0x4aa7e003, 0x7c74, 0x11cf, \
				0xb5, 0x00, 0x08, 0x00, 0x09, 0x53, 0xad, 0xc2)

#endif /* _OSKIT_MIB_IP_H_ */

/* End of file. */

