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
#ifndef _OSKIT_FLASK_AVC_SS_H_
#define _OSKIT_FLASK_AVC_SS_H_

#include <oskit/com.h>
#include <flask/flask.h>
#include <flask/av_permissions.h>

/*
 * COM access vector cache interface for the security server.
 * IID 4aa7dffa-7c74-11cf-b500-08000953adc2
 */

struct oskit_avc_ss {
        struct oskit_avc_ss_ops *ops;
};
typedef struct oskit_avc_ss oskit_avc_ss_t;

struct oskit_avc_ss_ops {

	/*** COM-specified IUnknown interface operations ***/
	OSKIT_COMDECL	(*query)(oskit_avc_ss_t *a,
				 const struct oskit_guid *iid,
				 void **out_ihandle);
	OSKIT_COMDECL_U	(*addref)(oskit_avc_ss_t *a);
	OSKIT_COMDECL_U	(*release)(oskit_avc_ss_t *a);

	/*** Operations specific to the oskit_avc_ss interface ***/

	OSKIT_COMDECL (*grant)(oskit_avc_ss_t *a,
			       oskit_security_id_t ssid,		
			       oskit_security_id_t tsid,	       
			       oskit_security_class_t tclass,
			       oskit_access_vector_t perms,
			       oskit_u32_t seqno);

	OSKIT_COMDECL (*try_revoke)(oskit_avc_ss_t *a,
				    oskit_security_id_t ssid,		
				    oskit_security_id_t tsid,	       
				    oskit_security_class_t tclass,
				    oskit_access_vector_t perms,
				    oskit_u32_t seqno,
				    oskit_access_vector_t *out_retained);

	OSKIT_COMDECL (*revoke)(oskit_avc_ss_t *a,
			       oskit_security_id_t ssid,		
			       oskit_security_id_t tsid,	       
			       oskit_security_class_t tclass,
			       oskit_access_vector_t perms,
			       oskit_u32_t seqno);

	OSKIT_COMDECL (*reset)(oskit_avc_ss_t *a,
			       oskit_u32_t seqno);

	OSKIT_COMDECL (*set_auditallow)(oskit_avc_ss_t *a,
					oskit_security_id_t ssid,
					oskit_security_id_t tsid,
					oskit_security_class_t tclass,
					oskit_access_vector_t perms,
					oskit_u32_t seqno,
					oskit_bool_t enable);

	OSKIT_COMDECL (*set_auditdeny)(oskit_avc_ss_t *a,
					oskit_security_id_t ssid,
					oskit_security_id_t tsid,
					oskit_security_class_t tclass,
					oskit_access_vector_t perms,
					oskit_u32_t seqno,
					oskit_bool_t enable);
	
	OSKIT_COMDECL (*set_notify)(oskit_avc_ss_t *a,
				oskit_security_id_t ssid,
				oskit_security_id_t tsid,
				oskit_security_class_t tclass,
				oskit_access_vector_t perms,
				oskit_u32_t seqno,
				oskit_bool_t enable);
};

#define oskit_avc_ss_query(a,iid,out_ihandle) ((a)->ops->query((a),(iid),(out_ihandle)))
#define oskit_avc_ss_addref(a) ((a)->ops->addref((a)))
#define oskit_avc_ss_release(a) ((a)->ops->release((a)))
#define oskit_avc_ss_grant(a,ssid,tsid,tclass,perms,seqno) ((a)->ops->grant((a),(ssid),(tsid),(tclass),(perms),(seqno)))
#define oskit_avc_ss_try_revoke(a,ssid,tsid,tclass,perms,seqno,out_retained) ((a)->ops->try_revoke((a),(ssid),(tsid),(tclass),(perms),(seqno),(out_retained)))
#define oskit_avc_ss_revoke(a,ssid,tsid,tclass,perms,seqno) ((a)->ops->revoke((a),(ssid),(tsid),(tclass),(perms),(seqno)))
#define oskit_avc_ss_reset(a,seqno) ((a)->ops->reset((a),(seqno)))
#define oskit_avc_ss_set_auditallow(a,ssid,tsid,tclass,perms,seqno,enable) ((a)->ops->set_auditallow((a),(ssid),(tsid),(tclass),(perms),(seqno),(enable)))
#define oskit_avc_ss_set_auditdeny(a,ssid,tsid,tclass,perms,seqno,enable) ((a)->ops->set_auditdeny((a),(ssid),(tsid),(tclass),(perms),(seqno),(enable)))
#define oskit_avc_ss_set_notify(a,ssid,tsid,tclass,perms,seqno,enable) ((a)->ops->set_notify((a),(ssid),(tsid),(tclass),(perms),(seqno),(enable)))

/* GUID for oskit_avc_ss interface */
extern const struct oskit_guid oskit_avc_ss_iid;
#define OSKIT_AVC_SS_IID OSKIT_GUID(0x4aa7dffa, 0x7c74, 0x11cf, \
				0xb5, 0x00, 0x08, 0x00, 0x09, 0x53, 0xad, 0xc2)

#endif
