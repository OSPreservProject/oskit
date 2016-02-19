/*
 * Copyright (c) 1999 The University of Utah and the Flux Group.
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

#ifndef _OSKIT_FLASK_AVC_H_
#define _OSKIT_FLASK_AVC_H_

#include <oskit/com.h>
#include <oskit/boolean.h>
#include <oskit/com/services.h>
#include <flask/flask.h>
#include <flask/av_permissions.h>
#include <oskit/flask/avc_callback.h>
#include <oskit/flask/security.h>

/*
 * COM access vector cache interface.
 * IID 4aa7dfe1-7c74-11cf-b500-08000953adc2
 */

typedef struct oskit_avc_entry {
	oskit_security_id_t   ssid;
	oskit_security_id_t   tsid;
	oskit_security_class_t tclass;
	oskit_access_vector_t allowed;       
	oskit_access_vector_t decided; 
        oskit_access_vector_t auditallow;
	oskit_access_vector_t auditdeny;
	oskit_access_vector_t notify;
	oskit_bool_t	      used;
} oskit_avc_entry_t;

typedef struct oskit_avc_entry_ref { 
	oskit_avc_entry_t *ae;	
} oskit_avc_entry_ref_t;

#define OSKIT_AVC_ENTRY_REF_NULL { 0 }
#define OSKIT_AVC_ENTRY_REF_INIT(h) { (h)->ae = 0; }
#define OSKIT_AVC_ENTRY_REF_CPY(dst,src) (dst)->ae = (src)->ae

#define OSKIT_AVC_AUDITALLOW 0
#define OSKIT_AVC_AUDITDENY  1

#define OSKIT_AVC_KEEP_STATS	       1 
#if OSKIT_AVC_KEEP_STATS
#define OSKIT_AVC_ENTRY_LOOKUPS        0
#define OSKIT_AVC_ENTRY_HITS	       1
#define OSKIT_AVC_ENTRY_MISSES         2
#define OSKIT_AVC_ENTRY_DISCARDS       3
#define OSKIT_AVC_CAV_LOOKUPS          4
#define OSKIT_AVC_CAV_HITS             5
#define OSKIT_AVC_CAV_PROBES           6
#define OSKIT_AVC_CAV_MISSES           7
#define OSKIT_AVC_NSTATS               8
#endif /* OSKIT_AVC_KEEP_STATS */

#define OSKIT_AVC_ALWAYS_ALLOW		1

struct oskit_avc {
        struct oskit_avc_ops *ops;
	oskit_security_t *security;
#if OSKIT_AVC_KEEP_STATS
        unsigned stats[OSKIT_AVC_NSTATS];
#endif
#if OSKIT_AVC_ALWAYS_ALLOW
	oskit_bool_t always_allow;
#endif	
};
typedef struct oskit_avc oskit_avc_t;

struct oskit_avc_ops {

	/*** COM-specified IUnknown interface operations ***/
	OSKIT_COMDECL	(*query)(oskit_avc_t *a,
				 const struct oskit_guid *iid,
				 void **out_ihandle);
	OSKIT_COMDECL_U	(*addref)(oskit_avc_t *a);
	OSKIT_COMDECL_U	(*release)(oskit_avc_t *a);

	/*** Operations specific to the oskit_avc interface ***/

	OSKIT_COMDECL (*lookup)(oskit_avc_t *a,
			    oskit_security_id_t ssid,		
			    oskit_security_id_t tsid,	       
			    oskit_security_class_t tclass,
			    oskit_access_vector_t requested,
			    oskit_avc_entry_ref_t *out_aeref);
	
	OSKIT_COMDECL (*insert)(oskit_avc_t *a,
			    oskit_security_id_t ssid,		
			    oskit_security_id_t tsid,	       
			    oskit_security_class_t tclass,
			    oskit_avc_entry_t *ae,
			    oskit_u32_t seqno,
			    oskit_avc_entry_ref_t *out_aeref);

	OSKIT_COMDECL_V (*audit)(oskit_avc_t *a,
				 oskit_security_id_t ssid,		
				 oskit_security_id_t tsid,	       
				 oskit_security_class_t tclass,
				 oskit_access_vector_t requested,
				 oskit_avc_entry_t *ae,
				 oskit_u32_t denied);

	OSKIT_COMDECL	(*add_callback)(oskit_avc_t *a,
				        struct oskit_avc_callback *c,
					oskit_u32_t events,
                                        oskit_security_id_t ssid,
                                        oskit_security_id_t tsid,
                                        oskit_security_class_t tclass,
				        oskit_access_vector_t perms);

        OSKIT_COMDECL	(*remove_callback)(oskit_avc_t *a,
					   struct oskit_avc_callback *c);
  
	OSKIT_COMDECL_V	(*log_contents)(oskit_avc_t *a, int priority, char *tag);

	OSKIT_COMDECL_V	(*log_stats)(oskit_avc_t *a, int priority, char *tag);
};


#define oskit_avc_query(a,iid,out_ihandle) ((a)->ops->query((a),(iid),(out_ihandle)))
#define oskit_avc_addref(a) ((a)->ops->addref((a)))
#define oskit_avc_release(a) ((a)->ops->release((a)))
#define oskit_avc_lookup(a,ssid,tsid,tclass,requested,aeref) ((a)->ops->lookup((a),(ssid),(tsid),(tclass),(requested),(aeref)))
#define oskit_avc_insert(a,ssid,tsid,tclass,ae,seqno,aeref) ((a)->ops->insert((a),(ssid),(tsid),(tclass),(ae),(seqno),(aeref)))
#define oskit_avc_audit(a,ssid,tsid,tclass,requested,ae,denied) ((a)->ops->audit((a),(ssid),(tsid),(tclass),(requested),(ae),(denied)))
#define oskit_avc_add_callback(a,c,events,ssid,tsid,tclass,perms) ((a)->ops->add_callback((a),(c),(events),(ssid),(tsid),(tclass),(perms)))
#define oskit_avc_remove_callback(a,c) ((a)->ops->add_callback((a),(c)))
#define oskit_avc_log_contents(a,pri,tag) ((a)->ops->log_contents((a),(pri),(tag)))
#define oskit_avc_log_stats(a,pri,tag) ((a)->ops->log_stats((a),(pri),(tag)))


extern inline OSKIT_COMDECL oskit_avc_get_ref(
	struct oskit_avc *avc,	
	oskit_security_id_t ssid,
	oskit_security_id_t tsid,
	oskit_security_class_t tclass,
	oskit_access_vector_t requested,
	oskit_avc_entry_ref_t *aeref)
{
	struct oskit_avc_entry *ae;
	int             rc;
	struct oskit_avc_entry entry;
        oskit_u32_t seqno;

#if OSKIT_AVC_KEEP_STATS
	avc->stats[OSKIT_AVC_ENTRY_LOOKUPS]++;
#endif

	ae = aeref->ae;
	if (ae) {
		if (ae->ssid == ssid &&
		    ae->tsid == tsid &&
		    ae->tclass == tclass &&
		    ((ae->decided & requested) == requested)) {
#if OSKIT_AVC_KEEP_STATS
			avc->stats[OSKIT_AVC_ENTRY_HITS]++;
#endif 
			ae->used = TRUE;
		} else {
#if OSKIT_AVC_KEEP_STATS
			avc->stats[OSKIT_AVC_ENTRY_DISCARDS]++;
#endif 
			ae = 0;
		}
	}

	if (!ae) {
#if OSKIT_AVC_KEEP_STATS
		avc->stats[OSKIT_AVC_ENTRY_MISSES]++;
#endif
		rc = oskit_avc_lookup(avc, ssid, tsid, tclass, requested, 
					aeref);
		if (rc) { 
			rc = oskit_security_compute_av(avc->security, 
						 ssid, tsid, tclass, requested,
					         &entry.allowed,
					         &entry.decided,
						 &entry.auditallow,
						 &entry.auditdeny,
						 &entry.notify,
						 &seqno);
			if (rc)
				return rc;
			rc = oskit_avc_insert(avc, ssid, tsid, tclass, 
						&entry, seqno, aeref);
			if (rc) 
				return rc;	
		}
	}
	
	return 0;
}


extern inline OSKIT_COMDECL oskit_avc_has_perm_ref(
	struct oskit_avc *avc,
	oskit_security_id_t ssid,
	oskit_security_id_t tsid,
	oskit_security_class_t tclass,
	oskit_access_vector_t requested,
	oskit_avc_entry_ref_t *aeref)
{
	struct oskit_avc_entry *ae;
	int             rc;


        rc = oskit_avc_get_ref(avc,ssid,tsid,tclass,requested,aeref);
	if (rc)
                return rc;	  

	ae = aeref->ae;

#if OSKIT_AVC_ALWAYS_ALLOW
	if (avc->always_allow) {
		if ((requested & ae->allowed) != requested) {
			oskit_avc_audit(avc, ssid, tsid, tclass, 
					requested, ae, OSKIT_AVC_AUDITDENY);
			ae->allowed |= requested;
		}
	}
#endif

	if ((requested & ae->allowed) != requested) {
		if (requested & ae->auditdeny)
			oskit_avc_audit(avc, ssid, tsid, tclass, 
					requested, ae, OSKIT_AVC_AUDITDENY);
		return OSKIT_EACCES;
	}

	if (requested & ae->auditallow)
		oskit_avc_audit(avc, ssid, tsid, tclass, requested, 
				ae, OSKIT_AVC_AUDITALLOW);

	return 0;    
}


extern inline OSKIT_COMDECL oskit_avc_has_perm(
	struct oskit_avc *avc,		/* IN */
	oskit_security_id_t ssid,	   /* IN */
	oskit_security_id_t tsid,	   /* IN */
	oskit_security_class_t tclass,   /* IN */
	oskit_access_vector_t requested) /* IN */
{
	oskit_avc_entry_ref_t ref;
	OSKIT_AVC_ENTRY_REF_INIT(&ref);
	return oskit_avc_has_perm_ref(avc,ssid,tsid,tclass,requested,&ref);
}


extern inline OSKIT_COMDECL oskit_avc_notify_perm_ref(
	struct oskit_avc *avc,		/* IN */
	oskit_security_id_t ssid,		/* IN */
	oskit_security_id_t tsid,		/* IN */
	oskit_security_class_t tclass,	/* IN */
	oskit_access_vector_t requested, 	/* IN */
	oskit_avc_entry_ref_t *aeref)		/* IN */
{
	struct oskit_avc_entry *ae;
	int             rc;

        rc = oskit_avc_get_ref(avc,ssid,tsid,tclass,requested,aeref);
	if (rc)
                return rc;	  

	ae = aeref->ae;

	if (requested & ae->notify) {
		return oskit_security_notify_perm(avc->security,
						  ssid,tsid,tclass,requested);
	}

	return 0;    
}


extern inline OSKIT_COMDECL oskit_avc_notify_perm(
	struct oskit_avc *avc,		/* IN */
	oskit_security_id_t ssid,	   /* IN */
	oskit_security_id_t tsid,	   /* IN */
	oskit_security_class_t tclass,   /* IN */
	oskit_access_vector_t requested) /* IN */
{
	oskit_avc_entry_ref_t ref;
	OSKIT_AVC_ENTRY_REF_INIT(&ref);
	return oskit_avc_notify_perm_ref(avc,ssid,tsid,tclass,requested,&ref);
}


oskit_error_t oskit_avc_create(oskit_services_t *osenv,
			       struct oskit_security *security,
			       oskit_avc_t **out_avc);


/* GUID for oskit_avc interface */
extern const struct oskit_guid oskit_avc_iid;
#define OSKIT_AVC_IID OSKIT_GUID(0x4aa7dfe1, 0x7c74, 0x11cf, \
				0xb5, 0x00, 0x08, 0x00, 0x09, 0x53, 0xad, 0xc2)

#endif 
