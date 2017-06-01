/*
 * Copyright (c) 1997,1999 The University of Utah and the Flux Group.
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

#ifndef _OSKIT_COMSID_H
#define _OSKIT_COMSID_H 1

#include <oskit/com.h>
#include <oskit/flask/flask_types.h>

/*
 * COM object SID interface.
 * IID 4aa7dfe0-7c74-11cf-b500-08000953adc2
 *
 * Get or set the SID of a COM object.
 *
 * The MOM<->COM adaptors support this interface on
 * the client call context object, for use by either
 * the security COM layers or by the external object
 * COM layers.
 *
 * The security COM layers support this interface
 * on each of the objects which they manage, for use
 * by the MOM<->COM adaptors.
 */
struct oskit_comsid {
        struct oskit_comsid_ops *ops;
};
typedef struct oskit_comsid oskit_comsid_t;


struct oskit_comsid_ops {

	/*** COM-specified IUnknown interface operations ***/
	OSKIT_COMDECL	(*query)(oskit_comsid_t *s,
				 const struct oskit_guid *iid,
				 void **out_ihandle);
	OSKIT_COMDECL_U	(*addref)(oskit_comsid_t *s);
	OSKIT_COMDECL_U	(*release)(oskit_comsid_t *s);

	/*** Operations specific to the oskit_comsid interface ***/

	/*
	 * Obtain this object's SID.
	 */
	OSKIT_COMDECL	(*get)(oskit_comsid_t *s,
			       oskit_security_class_t *outclass,
			       oskit_security_id_t *outsid);

	/*
	 * Change this object's SID to the specified value.
	 *
	 * This method is optional; an object may
	 * return OSKIT_E_NOTIMPL if the object does
	 * not support non-tranquility.
	 */
	OSKIT_COMDECL	(*set)(oskit_comsid_t *s,
			       oskit_security_id_t newsid);
};


#define oskit_comsid_query(s,iid,out_ihandle) ((s)->ops->query((s),(iid),(out_ihandle)))
#define oskit_comsid_addref(s) ((s)->ops->addref((s)))
#define oskit_comsid_release(s) ((s)->ops->release((s)))
#define oskit_comsid_get(s, outclass, outsid) ((s)->ops->get((s),(outclass),(outsid)))
#define oskit_comsid_set(s, newsid) ((s)->ops->set((s),(newsid)))


#define CSID(_csidp) { \
    oskit_comsid_t *sidi; \
    oskit_security_class_t sclass; \
    oskit_error_t rc; \
    rc = oskit_get_call_context(&oskit_comsid_iid, (void*)&sidi); \
    if (rc) return rc; \
    rc = oskit_comsid_get(sidi, &sclass, _csidp); \
    oskit_comsid_release(sidi); \
    if (rc) { return rc; } \
}

/* GUID for oskit_comsid interface */
extern const struct oskit_guid oskit_comsid_iid;
#define OSKIT_COMSID_IID OSKIT_GUID(0x4aa7dfe0, 0x7c74, 0x11cf, \
				0xb5, 0x00, 0x08, 0x00, 0x09, 0x53, 0xad, 0xc2)

#endif
