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
 * Definition of the COM-based `system' MIB interface.
 */
#ifndef _OSKIT_MIB_SYSTEM_H_
#define _OSKIT_MIB_SYSTEM_H_

#include <oskit/com.h>
#include <oskit/mib.h>

/*
 * Struct communicated by `get_sysstats'.
 */
typedef struct oskit_mib_slotset_sysstats oskit_mib_slotset_sysstats_t;
struct oskit_mib_slotset_sysstats {
	/*
	 * These slots are pretty much verbatim from the SNMPv2 MIB-II `system'
	 * group, except for `sysObjectID'.  For each slot, the SNMP type is
	 * shown.  See IETF RFC 1213.
	 */
	
	/* DisplayString(SIZE(0..255)) */
	oskit_mib_mib_string_t			sys_descr;
	
	/* OBJECT IDENTIFIER */
	/*					sys_object_id; */
	
	/* TimeTicks */
	oskit_mib_u32_t				sys_up_time;
	
	/* DisplayString(SIZE(0..255)) */
	oskit_mib_mib_string_t			sys_contact;
	
	/* DisplayString(SIZE(0..255)) */
	oskit_mib_mib_string_t			sys_name;
	
	/* DisplayString(SIZE(0..255)) */
	oskit_mib_mib_string_t			sys_location;
	
	/* INTEGER(0..127) */
	oskit_mib_u8_t				sys_services;
};

/*
 * System MIB interface.
 * IID 4aa7e000-7c74-11cf-b500-08000953adc2
 */
struct oskit_mib_system {
	struct oskit_mib_system_ops *ops;
};
typedef struct oskit_mib_system oskit_mib_system_t;

struct oskit_mib_system_ops {
	/*
	 * COM-specified IUnknown interface operations.
	 */
	OSKIT_COMDECL_IUNKNOWN(oskit_mib_system_t)

	/*
	 * Operations specific to the system monitor interface.
	 */
	OSKIT_MIB_SLOTSET_OPS_RO	(oskit_mib_system_t,
					 oskit_mib_slotset_sysstats_t,
					 sysstats)
	/* SNMP does not require `sysDescr' to be writable. */
	OSKIT_MIB_SLOT_OPS_RW		(oskit_mib_system_t,
					 mib_string,
					 sys_descr)
	OSKIT_MIB_SLOT_OPS_RW		(oskit_mib_system_t,
					 mib_string,
					 sys_contact)
	OSKIT_MIB_SLOT_OPS_RW		(oskit_mib_system_t,
					 mib_string,
					 sys_name)
	OSKIT_MIB_SLOT_OPS_RW		(oskit_mib_system_t,
					 mib_string,
					 sys_location)
};

#define oskit_mib_system_query(sys, iid, out_ihandle) \
	((sys)->ops->query((oskit_mib_system_t *)(sys), (iid), (out_ihandle)))
#define oskit_mib_system_addref(sys) \
	((sys)->ops->addref((oskit_mib_system_t *)(sys)))
#define oskit_mib_system_release(sys) \
	((sys)->ops->release((oskit_mib_system_t *)(sys)))

#define oskit_mib_system_get_sysstats(sys, sysstats) \
	((sys)->ops->get_system_stats((oskit_mib_system_t *)(sys), (sysstats)))

#define oskit_mib_system_get_sys_descr(sys, des) \
	((sys)->ops->get_sys_descr((oskit_mib_system_t *)(sys), (des)))
#define oskit_mib_system_set_sys_descr(sys, des) \
	((sys)->ops->set_sys_descr((oskit_mib_system_t *)(sys), (des)))

#define oskit_mib_system_get_sys_contact(sys, con) \
	((sys)->ops->get_sys_contact((oskit_mib_system_t *)(sys), (con)))
#define oskit_mib_system_set_sys_contact(sys, con) \
	((sys)->ops->set_sys_contact((oskit_mib_system_t *)(sys), (con)))

#define oskit_mib_system_get_sys_name(sys, name) \
	((sys)->ops->get_sys_name((oskit_mib_system_t *)(sys), (name)))
#define oskit_mib_system_set_sys_name(sys, con) \
	((sys)->ops->set_sys_name((oskit_mib_system_t *)(sys), (name)))

#define oskit_mib_system_get_sys_location(sys, loc) \
	((sys)->ops->get_sys_location((oskit_mib_system_t *)(sys), (loc)))
#define oskit_mib_system_set_sys_location(sys, loc) \
	((sys)->ops->set_sys_location((oskit_mib_system_t *)(sys), (loc)))

/* GUID for oskit_mib_system interface */
extern const struct oskit_guid oskit_mib_system_iid;
#define OSKIT_MIB_SYSTEM_IID OSKIT_GUID(0x4aa7e000, 0x7c74, 0x11cf, \
				0xb5, 0x00, 0x08, 0x00, 0x09, 0x53, 0xad, 0xc2)

#endif /* _OSKIT_MIB_SYSTEM_H_ */

/* End of file. */

