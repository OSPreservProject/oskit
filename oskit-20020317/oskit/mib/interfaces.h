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
 * Definition of the COM-based `interfaces' MIB interface.
 */
#ifndef _OSKIT_MIB_INTERFACES_H_
#define _OSKIT_MIB_INTERFACES_H_

#include <oskit/com.h>
#include <oskit/mib.h>

/*
 * Struct communicated by `get_ifstats'.
 */
typedef struct oskit_mib_slotset_ifstats oskit_mib_slotset_ifstats_t;
struct oskit_mib_slotset_ifstats {
	/*
	 * These slots are pretty much verbatim from the SNMPv2 MIB-II
	 * `interfaces' group, minus the tables.  For each slot, the SNMP type
	 * is shown.  See IETF RFC 2863.  (Actually, RFC 1213 is much closer to
	 * the current implementation.)
	 */
	
	/* INTEGER        */ oskit_mib_s32_t	if_number;
	
	/* SEQUENCE OF IfEntry			if_table; */
	
	/*********************************************************************/
	
	/*
	 * RFC 2863 defines these objects under the `ifMIB' group.  `ifMIB' is
	 * a sibling of `interfaces': the `interfaces' OID is { mib-2 2 } and
	 * the `ifMIB' OID is { mib-2 31 }.
	 */
	
	/* SEQUENCE OF IfXEntry			if_x_table; */
	/* SEQUENCE OF IfStackEntry		if_stack_table; */
	/* (obsolete)				if_test_table; */
	/* SEQUENCE OF IfRcvAddressEntry	if_rcv_address_table; */
	/* TimeTicks				if_table_last_change; */
	/* TimeTicks				if_stack_last_change; */
};

/*
 * Struct communicated by `get_ifentry_table'.
 */
typedef struct oskit_mib_slotset_ifentry oskit_mib_slotset_ifentry_t;
struct oskit_mib_slotset_ifentry {
	/*
	 * These slots are pretty much verbatim from the SNMPv2 MIB-II
	 * `interfaces' group's `ifEntry' sequence.  For each slot, the SNMP
	 * type is shown.  See IETF RFC 2863.
	 */
	
	/* InterfaceIndex */ oskit_mib_u32_t		if_index;
	
	/* DisplayString(SIZE(0..255)) */
	oskit_mib_mib_string_t				if_descr;

	/* IANAifType    */ oskit_mib_u32_t		if_type;
	/* Integer32     */ oskit_mib_s32_t		if_mtu;
	/* Gauge32       */ oskit_mib_u32_t		if_speed;
	/* PhysAddress   */ oskit_mib_mib_string_t	if_phys_address;
	/* INTEGER(1..3) */ oskit_mib_u32_t		if_admin_status;
	/* INTEGER(1..7) */ oskit_mib_u32_t		if_oper_status;
	/* TimeTicks     */ oskit_mib_u32_t		if_last_change;
	
	/* Counter32     */ oskit_mib_u32_t		if_in_octets;
	/* Counter32     */ oskit_mib_u32_t		if_in_ucast_pkts;
	/* Counter32     */ oskit_mib_u32_t		if_in_n_ucast_pkts;
	/* Counter32     */ oskit_mib_u32_t		if_in_discards;
	/* Counter32     */ oskit_mib_u32_t		if_in_errors;
	/* Counter32     */ oskit_mib_u32_t		if_in_unknown_protos;
	
	/* Counter32     */ oskit_mib_u32_t		if_out_octets;
	/* Counter32     */ oskit_mib_u32_t		if_out_ucast_pkts;
	/* Counter32     */ oskit_mib_u32_t		if_out_n_ucast_pkts;
	/* Counter32     */ oskit_mib_u32_t		if_out_discards;
	/* Counter32     */ oskit_mib_u32_t		if_out_errors;
	/* Gauge32       */ oskit_mib_u32_t		if_out_q_len;
	
	/* OBJECT IDENTIFIER				if_specific; */
};

/*
 * Struct communicated by `get_ifentry_status_table'.
 */
typedef struct oskit_mib_slotset_ifentry_status
	oskit_mib_slotset_ifentry_status_t;
struct oskit_mib_slotset_ifentry_status {
	/*
	 * This is a projection of selected slots from the full `ifentry'
	 * table, intended to make it easier for programs to change the status
	 * of interfaces.
	 */
	
	/* InterfaceIndex */ oskit_mib_u32_t		if_index;
	
	/* IANAifType    */ oskit_mib_u32_t		if_type;
	/* PhysAddress   */ oskit_mib_mib_string_t	if_phys_address;
	/* INTEGER(1..3) */ oskit_mib_u32_t		if_admin_status;
};

/*
 * Interfaces MIB interface.
 * IID 4aa7e001-7c74-11cf-b500-08000953adc2
 */
struct oskit_mib_interfaces {
	struct oskit_mib_interfaces_ops *ops;
};
typedef struct oskit_mib_interfaces oskit_mib_interfaces_t;

struct oskit_mib_interfaces_ops {
	/*
	 * COM-specified IUnknown interface operations.
	 */
	OSKIT_COMDECL_IUNKNOWN(oskit_mib_interfaces_t)

	/*
	 * Operations specific to the interfaces monitor interface.
	 *
	 * In the SNMPv2 MIB-II `interfaces' group, only the `ifAdminStatus'
	 * column of the `ifTable' is read-write.  All of the other slots are
	 * read-only.
	 */
	OSKIT_MIB_SLOTSET_OPS_RO	(oskit_mib_interfaces_t,
					 oskit_mib_slotset_ifstats_t,
					 ifstats)
	OSKIT_MIB_SLOTSET_TABLE_OPS_RO	(oskit_mib_interfaces_t,
					 oskit_mib_slotset_ifentry_t,
					 ifentry)
	OSKIT_MIB_SLOTSET_TABLE_OPS_RW	(oskit_mib_interfaces_t,
					 oskit_mib_slotset_ifentry_status_t,
					 ifentry_status)
};

#define oskit_mib_interfaces_query(in, iid, out_ihandle)	\
	((in)->ops->query((oskit_mib_interfaces_t *)(in),	\
			  (iid),				\
			  (out_ihandle)))
#define oskit_mib_interfaces_addref(in) \
	((in)->ops->addref((oskit_mib_interfaces_t *)(in)))
#define oskit_mib_interfaces_release(in) \
	((in)->ops->release((oskit_mib_interfaces_t *)(in)))

#define oskit_mib_interfaces_get_ifstats(in, ifstats) \
	((in)->ops->get_ifstats((oskit_mib_interfaces_t *)(in), (ifstats)))

#define oskit_mib_interfaces_get_ifentry_table(in,			   \
					       start_row, want_rows,	   \
					       matching,		   \
					       table, out_rows, more_rows) \
	((in)->ops->get_ifentry_table((oskit_mib_interfaces_t *)(in),	   \
				      (start_row),			   \
				      (want_rows),			   \
				      (matching),			   \
				      /* OUT */ (table),		   \
				      /* OUT */ (out_rows),		   \
				      /* OUT */ (more_rows)))

#define oskit_mib_interfaces_get_ifentry_status_table(in,		     \
						      start_row, want_rows,  \
						      matching,		     \
						      table,		     \
						      out_rows, more_rows)   \
	((in)->ops->get_ifentry_status_table((oskit_mib_interfaces_t *)(in), \
					     (start_row),		     \
					     (want_rows),		     \
					     (matching),		     \
					     /* OUT */ (table),		     \
					     /* OUT */ (out_rows),	     \
					     /* OUT */ (more_rows)))
#define oskit_mib_interfaces_set_ifentry_status_table(in,		     \
						      matching, all_matches, \
						      vals)		     \
	((in)->ops->get_ifentry_status_table((oskit_mib_interfaces_t *)(in), \
					     (matching),		     \
					     (all_matches),		     \
					     (vals)))

/* GUID for oskit_mib_interfaces interface */
extern const struct oskit_guid oskit_mib_interfaces_iid;
#define OSKIT_MIB_INTERFACES_IID OSKIT_GUID(0x4aa7e001, 0x7c74, 0x11cf, \
				0xb5, 0x00, 0x08, 0x00, 0x09, 0x53, 0xad, 0xc2)

#endif /* _OSKIT_MIB_INTERFACES_H_ */

/* End of file. */

