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

#ifndef _OSKIT_FLASK_AVC_CALLBACK_H
#define _OSKIT_FLASK_AVC_CALLBACK_H 

#include <oskit/com.h>
#include <flask/flask.h>

/*
 * COM access vector cache callback interface.
 * IID 4aa7dfe2-7c74-11cf-b500-08000953adc2
 */

#define OSKIT_AVC_CALLBACK_GRANT		1
#define OSKIT_AVC_CALLBACK_TRY_REVOKE		2
#define OSKIT_AVC_CALLBACK_REVOKE		4
#define OSKIT_AVC_CALLBACK_RESET		8
#define OSKIT_AVC_CALLBACK_AUDITALLOW_ENABLE	16
#define OSKIT_AVC_CALLBACK_AUDITALLOW_DISABLE	32
#define OSKIT_AVC_CALLBACK_AUDITDENY_ENABLE	64
#define OSKIT_AVC_CALLBACK_AUDITDENY_DISABLE	128
#define OSKIT_AVC_CALLBACK_NOTIFY_ENABLE        256
#define OSKIT_AVC_CALLBACK_NOTIFY_DISABLE	512

struct oskit_avc_callback {
        struct oskit_avc_callback_ops *ops;
};
typedef struct oskit_avc_callback oskit_avc_callback_t;


struct oskit_avc_callback_ops {

	/*** COM-specified IUnknown interface operations ***/
	OSKIT_COMDECL	(*query)(oskit_avc_callback_t *c,
				 const struct oskit_guid *iid,
				 void **out_ihandle);
	OSKIT_COMDECL_U	(*addref)(oskit_avc_callback_t *c);
	OSKIT_COMDECL_U	(*release)(oskit_avc_callback_t *c);

	/*** Operations specific to the oskit_avc_callback interface ***/

	OSKIT_COMDECL	(*invoke)(oskit_avc_callback_t *c,
				  oskit_u32_t event,
				  oskit_security_id_t ssid,
				  oskit_security_id_t tsid,
				  oskit_security_class_t tclass,
				  oskit_access_vector_t perms,
				  oskit_access_vector_t *out_retained);
};


#define oskit_avc_callback_query(c,iid,out_ihandle) ((c)->ops->query((c),(iid),(out_ihandle)))
#define oskit_avc_callback_addref(c) ((c)->ops->addref((c)))
#define oskit_avc_callback_release(c) ((c)->ops->release((c)))
#define oskit_avc_callback_invoke(c,events,ssid,tsid,tclass,perms,out_retained) ((c)->ops->invoke((c),(events),(ssid),(tsid),(tclass),(perms),(out_retained)))

/* GUID for oskit_avc_callback interface */
extern const struct oskit_guid oskit_avc_callback_iid;
#define OSKIT_AVC_CALLBACK_IID OSKIT_GUID(0x4aa7dfe2, 0x7c74, 0x11cf, \
				0xb5, 0x00, 0x08, 0x00, 0x09, 0x53, 0xad, 0xc2)

#endif
