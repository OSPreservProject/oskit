/* FLASK */

/*
 * Copyright (c) 1999, 2000 The University of Utah and the Flux Group.
 * All rights reserved.
 * 
 * Contributed by the Computer Security Research division,
 * INFOSEC Research and Technology Office, NSA.
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

#ifndef _SERVICES_H_
#define _SERVICES_H_

#include "policydb.h"
#include "sidtab.h"

/*
 * The security server uses two global data structures
 * when providing its services:  the SID table (sidtab)
 * and the policy database (policydb).
 */
extern sidtab_t sidtab;
extern policydb_t policydb;

/*
 * The prototypes for the security services.
 */

/*
 * Compute access vectors based on a SID pair for
 * the permissions in a particular class.
 */
int security_compute_av(
	security_id_t ssid,			/* IN */
	security_id_t tsid,			/* IN */
	security_class_t tclass,		/* IN */
	access_vector_t requested,		/* IN */
	access_vector_t *allowed,		/* OUT */
	access_vector_t *decided,		/* OUT */
#ifdef CONFIG_FLASK_AUDIT
	access_vector_t *auditallow,		/* OUT */
	access_vector_t *auditdeny,		/* OUT */
#endif
#ifdef CONFIG_FLASK_NOTIFY
	access_vector_t *notify,	       	/* OUT */
#endif
	__u32 *seqno);			/* OUT */

#ifdef CONFIG_FLASK_NOTIFY
/*
 * Notify the security server that an operation
 * associated with a previously granted permission 
 * has successfully completed.
 */
int security_notify_perm(
	security_id_t ssid,			/* IN */
	security_id_t tsid,			/* IN */
	security_class_t tclass,		/* IN */
	access_vector_t requested);		/* IN */
#endif

/*
 * Compute a SID to use for labeling a new object in the 
 * class `tclass' based on a SID pair.  
 */
int security_transition_sid(
	security_id_t ssid,			/* IN */
	security_id_t tsid,			/* IN */
	security_class_t tclass,		/* IN */
	security_id_t *out_sid);	        /* OUT */

/*
 * Compute a SID to use when selecting a member of a 
 * polyinstantiated object of class `tclass' based on 
 * a SID pair.
 */
int security_member_sid(
	security_id_t ssid,			/* IN */
	security_id_t tsid,			/* IN */
	security_class_t tclass,		/* IN */
	security_id_t *out_sid);	        /* OUT */

/*
 * Write the security context string representation of 
 * the context associated with `sid' into a dynamically
 * allocated string of the correct size.  Set `*scontext'
 * to point to this string and set `*scontext_len' to
 * the length of the string.
 */
int security_sid_to_context(
	security_id_t  sid,			/* IN */
	security_context_t *scontext,		/* OUT */
	__u32   *scontext_len);			/* OUT */

/*
 * Return a SID associated with the security context that
 * has the string representation specified by `scontext'.
 */
int security_context_to_sid(
	security_context_t scontext,		/* IN */
	__u32   scontext_len,			/* IN */
	security_id_t *out_sid);		/* OUT */

/*
 * Return the SIDs to use for an unlabeled file system
 * that is being mounted from the device with the
 * the kdevname `name'.  The `fs_sid' SID is returned for 
 * the file system and the `file_sid' SID is returned
 * for all files within that file system.
 */
int security_fs_sid(
	char *dev,				/* IN */
	security_id_t *fs_sid,			/* OUT  */
	security_id_t *file_sid);		/* OUT */

/*
 * Return the SID of the port specified by
 * `domain', `type', `protocol', and `port'.
 */
int security_port_sid(
	__u16 domain,
	__u16 type,
	__u8 protocol,
	__u16 port,
	security_id_t *out_sid);

/*
 * Return the SIDs to use for a network interface
 * with the name `name'.  The `if_sid' SID is returned for 
 * the interface and the `msg_sid' SID is returned as
 * the default SID for messages received on the
 * interface.
 */
int security_netif_sid(
	char *name,
	security_id_t *if_sid,
	security_id_t *msg_sid);

/*
 * Return the SID of the node specified by the address
 * `addr' where `addrlen' is the length of the address
 * in bytes and `domain' is the communications domain or
 * address family in which the address should be interpreted.
 */
int security_node_sid(
	__u16 domain,
	void *addr,
	__u32 addrlen,
	security_id_t *out_sid);

/*
 * Load a new policy configuration from `fp'.
 */
int security_load_policy(FILE * fp);	/* IN */


/*
 * This variable is set to one when the security server
 * has completed initialization.
 */
extern int ss_initialized;

#endif	/* _SERVICES_H_ */

/* FLASK */
