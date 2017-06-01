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
 * Definition of the COM-based SNMP MIB interface.
 */
#ifndef _OSKIT_MIB_SNMP_H_
#define _OSKIT_MIB_SNMP_H_

#include <oskit/com.h>
#include <oskit/mib.h>

/*
 * Struct communicated by `get_snmpstats'.
 */
typedef struct oskit_mib_slotset_snmpstats oskit_mib_slotset_snmpstats_t;
struct oskit_mib_slotset_snmpstats {
	/*
	 * These slots are pretty much verbatim from the SNMPv2 MIB-II `snmp'
	 * group.  For each slot, the SNMP type is shown.  See IETF RFC 1213.
	 */
	
	/* Counter       */ oskit_mib_u32_t	snmp_in_pkts;
	/* Counter       */ oskit_mib_u32_t	snmp_out_pkts;
	
	/* Counter       */ oskit_mib_u32_t	snmp_in_bad_versions;
	/* Counter       */ oskit_mib_u32_t	snmp_in_bad_community_names;
	/* Counter       */ oskit_mib_u32_t	snmp_in_bad_community_uses;
	/* Counter       */ oskit_mib_u32_t	snmp_in_asn_parse_errs;
	/* Counter       */ oskit_mib_u32_t	snmp_in_too_bigs;
	/* Counter       */ oskit_mib_u32_t	snmp_in_no_such_names;
	/* Counter       */ oskit_mib_u32_t	snmp_in_bad_values;
	/* Counter       */ oskit_mib_u32_t	snmp_in_read_onlys;
	/* Counter       */ oskit_mib_u32_t	snmp_in_gen_errs;
	/* Counter       */ oskit_mib_u32_t	snmp_in_total_req_vars;
	/* Counter       */ oskit_mib_u32_t	snmp_in_total_set_vars;
	/* Counter       */ oskit_mib_u32_t	snmp_in_get_requests;
	/* Counter       */ oskit_mib_u32_t	snmp_in_get_nexts;
	/* Counter       */ oskit_mib_u32_t	snmp_in_set_requests;
	/* Counter       */ oskit_mib_u32_t	snmp_in_get_responses;
	/* Counter       */ oskit_mib_u32_t	snmp_in_traps;
	
	/* Counter       */ oskit_mib_u32_t	snmp_out_too_bigs;
	/* Counter       */ oskit_mib_u32_t	snmp_out_no_such_names;
	/* Counter       */ oskit_mib_u32_t	snmp_out_bad_values;
	/* Counter       */ oskit_mib_u32_t	snmp_out_gen_errs;
	/* Counter       */ oskit_mib_u32_t	snmp_out_get_requests;
	/* Counter       */ oskit_mib_u32_t	snmp_out_get_nexts;
	/* Counter       */ oskit_mib_u32_t	snmp_out_set_requests;
	/* Counter       */ oskit_mib_u32_t	snmp_out_get_responses;
	/* Counter       */ oskit_mib_u32_t	snmp_out_traps;
	
	/* INTEGER(1..2) */ oskit_mib_bool_t	snmp_enable_authen_traps;
};

/*
 * SNMP MIB interface.
 * IID 4aa7e009-7c74-11cf-b500-08000953adc2
 */
struct oskit_mib_snmp {
	struct oskit_mib_snmp_ops *ops;
};
typedef struct oskit_mib_snmp oskit_mib_snmp_t;

struct oskit_mib_snmp_ops {
	/*
	 * COM-specified IUnknown interface operations.
	 */
	OSKIT_COMDECL_IUNKNOWN(oskit_mib_snmp_t)

	/*
	 * Operations specific to the SNMP monitor interface.
	 *
	 * In the SNMPv2 MIB-II `snmp' group, only `snmpEnableAuthenTraps' is
	 * read-write.  All of the other slots are read-only.
	 */
	OSKIT_MIB_SLOTSET_OPS_RO	(oskit_mib_snmp_t,
					 oskit_mib_slotset_snmpstats_t,
					 snmpstats)
	OSKIT_MIB_SLOT_OPS_RW		(oskit_mib_snmp_t,
					 u32,
					 snmp_enable_authen_traps)
};

#define oskit_mib_snmp_query(snmp, iid, out_ihandle) \
	((snmp)->ops->query((oskit_mib_snmp_t *)(snmp), (iid), (out_ihandle)))
#define oskit_mib_snmp_addref(snmp) \
	((snmp)->ops->addref((oskit_mib_snmp_t *)(snmp)))
#define oskit_mib_snmp_release(snmp) \
	((snmp)->ops->release((oskit_mib_snmp_t *)(snmp)))

#define oskit_mib_snmp_get_snmpstats(snmp, snmpstats) \
	((snmp)->ops->get_snmpstats((oskit_mib_snmp_t *)(snmp), (snmpstats)))

#define oskit_mib_snmp_get_snmp_enable_authen_traps(snmp, eat)	\
	((snmp)->ops->get_snmp_enable_authen_traps(		\
		(oskit_mib_snmp_t *)(snmp),			\
		(eat)))
#define oskit_mib_snmp_set_snmp_enable_authen_traps(snmp, eat)	\
	((snmp)->ops->set_snmp_enable_authen_traps(		\
		(oskit_mib_snmp_t *)(snmp),			\
		(eat)))

/* GUID for oskit_mib_snmp interface */
extern const struct oskit_guid oskit_mib_snmp_iid;
#define OSKIT_MIB_SNMP_IID OSKIT_GUID(0x4aa7e009, 0x7c74, 0x11cf, \
				0xb5, 0x00, 0x08, 0x00, 0x09, 0x53, 0xad, 0xc2)

#endif /* _OSKIT_MIB_SNMP_H_ */

/* End of file. */

