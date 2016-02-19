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
#ifndef _OSKIT_FLASK_SECURITY_H_
#define _OSKIT_FLASK_SECURITY_H_

#include <oskit/com.h>
#include <oskit/com/services.h>
#include <flask/flask.h>

/*
 * COM security server interface.
 * IID 4aa7dff9-7c74-11cf-b500-08000953adc2
 */

struct oskit_avc_ss;
struct oskit_openfile;

struct oskit_security {
        struct oskit_security_ops *ops;
};
typedef struct oskit_security oskit_security_t;

struct oskit_security_ops {

	/*** COM-specified IUnknown interface operations ***/
	OSKIT_COMDECL	(*query)(oskit_security_t *s,
				 const struct oskit_guid *iid,
				 void **out_ihandle);
	OSKIT_COMDECL_U	(*addref)(oskit_security_t *s);
	OSKIT_COMDECL_U	(*release)(oskit_security_t *s);

	/*** Operations specific to the oskit_security interface ***/

	OSKIT_COMDECL	(*compute_av)(oskit_security_t *s,
				      oskit_security_id_t ssid,
				      oskit_security_id_t tsid,
				      oskit_security_class_t tclass,
				      oskit_access_vector_t requested,
				      oskit_access_vector_t *allowed,
				      oskit_access_vector_t *decided,
				      oskit_access_vector_t *auditallow,
				      oskit_access_vector_t *auditdeny,
				      oskit_access_vector_t *notify,
				      oskit_u32_t *seqno);

	OSKIT_COMDECL	(*notify_perm)(oskit_security_t *s,
                                      oskit_security_id_t ssid,
                                      oskit_security_id_t tsid,
                                      oskit_security_class_t tclass,
                                      oskit_access_vector_t requested);

	OSKIT_COMDECL	(*transition_sid)(oskit_security_t *s,
				      oskit_security_id_t ssid,
				      oskit_security_id_t tsid,
				      oskit_security_class_t tclass,
				      oskit_security_id_t *out_sid);

	OSKIT_COMDECL	(*member_sid)(oskit_security_t *s,
				      oskit_security_id_t ssid,
				      oskit_security_id_t tsid,
				      oskit_security_class_t tclass,
				      oskit_security_id_t *out_sid);

	OSKIT_COMDECL	(*sid_to_context)(oskit_security_t *s,
					  oskit_security_id_t sid,
					  oskit_security_context_t *scontext,
					  oskit_u32_t *scontext_len);

	OSKIT_COMDECL	(*context_to_sid)(oskit_security_t *s,
					  oskit_security_context_t scontext,
					  oskit_u32_t scontext_len,
					  oskit_security_id_t *out_sid);

	OSKIT_COMDECL	(*register_avc)(oskit_security_t *s,
					oskit_security_class_t *classes,
					oskit_u32_t nclasses,
					struct oskit_avc_ss *avc);

	OSKIT_COMDECL	(*unregister_avc)(oskit_security_t *s,
					  struct oskit_avc_ss *avc);

	OSKIT_COMDECL	(*load_policy)(oskit_security_t *s,
				       struct oskit_openfile *f);

	OSKIT_COMDECL	(*fs_sid)(oskit_security_t *s,
				char *name,
				oskit_security_id_t *fs_sid,
				oskit_security_id_t *file_sid);

	OSKIT_COMDECL	(*port_sid)(oskit_security_t *s,
				oskit_u16_t domain,
				oskit_u16_t type,
				oskit_u8_t protocol,
				oskit_u16_t port,
				oskit_security_id_t *sid);

	OSKIT_COMDECL	(*netif_sid)(oskit_security_t *s,
				char *name,
				oskit_security_id_t *if_sid,
				oskit_security_id_t *msg_sid);

	OSKIT_COMDECL	(*node_sid)(oskit_security_t *s,
				oskit_u16_t domain,
				void *addr,
				oskit_u32_t addrlen,
				oskit_security_id_t *sid);
};

#define oskit_security_query(s,iid,out_ihandle) ((s)->ops->query((a),(iid),(out_ihandle)))
#define oskit_security_addref(s) ((s)->ops->addref((s)))
#define oskit_security_release(s) ((s)->ops->release((s)))
#define oskit_security_compute_av(s,ssid,tsid,tclass,requested,allowed,decided,auditallow,auditdeny,notify,seqno) ((s)->ops->compute_av((s),(ssid),(tsid),(tclass),(requested),(allowed),(decided),(auditallow),(auditdeny),(notify),(seqno)))
#define oskit_security_notify_perm(s,ssid,tsid,tclass,requested) ((s)->ops->notify_perm((s),(ssid),(tsid),(tclass),(requested)))
#define oskit_security_transition_sid(s,ssid,tsid,tclass,out_sid) ((s)->ops->transition_sid((s),(ssid),(tsid),(tclass),(out_sid)))
#define oskit_security_member_sid(s,ssid,tsid,tclass,out_sid) ((s)->ops->member_sid((s),(ssid),(tsid),(tclass),(out_sid)))
#define oskit_security_sid_to_context(s,sid,scontext,scontext_len) ((s)->ops->sid_to_context((s),(sid),(scontext),(scontext_len)))
#define oskit_security_context_to_sid(s,scontext,scontext_len,sid) ((s)->ops->context_to_sid((s),(scontext),(scontext_len),(sid)))
#define oskit_security_register_avc(s,classes,nclasses,avc) ((s)->ops->register_avc((s),(classes),(nclasses),(avc)))
#define oskit_security_load_policy(s,f) ((s)->ops->load_policy((s),(f)))
#define oskit_security_fs_sid(s,n,fs,file) ((s)->ops->fs_sid((s),(n),(fs),(file)))
#define oskit_security_port_sid(s,d,t,prot,port,sid) ((s)->ops->port_sid((s),(d),(t),(prot),(port),(sid)))
#define oskit_security_netif_sid(s,n,if,msg) ((s)->ops->netif_sid((s),(n),(if),(msg)))
#define oskit_security_node_sid(s,domain,addr,addrlen,sid) ((s)->ops->node_sid((s),(domain),(addr),(addrlen),(sid)))

oskit_error_t oskit_security_init(oskit_services_t *osenv,
				  struct oskit_openfile *f,
				  oskit_security_t **out_security);


/* GUID for oskit_security interface */
extern const struct oskit_guid oskit_security_iid;
#define OSKIT_SECURITY_IID OSKIT_GUID(0x4aa7dff9, 0x7c74, 0x11cf, \
				0xb5, 0x00, 0x08, 0x00, 0x09, 0x53, 0xad, 0xc2)

#endif
