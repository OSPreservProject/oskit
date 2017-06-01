/*
 * Copyright (c) 1997-1998 University of Utah and the Flux Group.
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
#ifndef _OSKIT_PRINCIPAL_H_
#define _OSKIT_PRINCIPAL_H_

#include <oskit/com.h>
#include <oskit/types.h>
#include <oskit/compiler.h>

OSKIT_BEGIN_DECLS

struct oskit_identity {
    	oskit_uid_t uid;			/* effective user id */
	oskit_gid_t gid;			/* effective group id */
	oskit_u32_t ngroups;		/* number of groups */
	oskit_u32_t *groups;		/* supplemental groups */
};
typedef struct oskit_identity oskit_identity_t;

/*
 * Principal interface.
 * IID 4aa7df84-7c74-11cf-b500-08000953adc2  
 */
struct oskit_principal {
        struct oskit_principal_ops *ops;
};
typedef struct oskit_principal oskit_principal_t;

struct oskit_principal_ops {

	/*** COM-specified IUnknown interface operations ***/
	OSKIT_COMDECL	(*query)(oskit_principal_t *p,
				 const struct oskit_guid *iid,
				 void **out_ihandle) OSKIT_STDCALL;
	OSKIT_COMDECL_U	(*addref)(oskit_principal_t *p) OSKIT_STDCALL;
	OSKIT_COMDECL_U	(*release)(oskit_principal_t *p) OSKIT_STDCALL;

	/*** Principal interface operations ***/

	OSKIT_COMDECL	(*getid)(oskit_principal_t *p,
				 oskit_identity_t *out_id) OSKIT_STDCALL;
};

#define oskit_principal_query(p, iid, out_ihandle) \
	((p)->ops->query((oskit_principal_t *)(p), (iid), (out_ihandle)))
#define oskit_principal_addref(p) \
	((p)->ops->addref((oskit_principal_t *)(p)))
#define oskit_principal_release(p) \
	((p)->ops->release((oskit_principal_t *)(p)))
#define oskit_principal_getid(p, out_id) \
	((p)->ops->getid((oskit_principal_t *)(p), (out_id)))

extern oskit_error_t oskit_principal_create(oskit_identity_t *id,
					  oskit_principal_t **out_prin);

/* GUID for oskit_principal interface */
extern const struct oskit_guid oskit_principal_iid;
#define OSKIT_PRINCIPAL_IID OSKIT_GUID(0x4aa7df84, 0x7c74, 0x11cf, \
				0xb5, 0x00, 0x08, 0x00, 0x09, 0x53, 0xad, 0xc2)

OSKIT_END_DECLS

#endif /* _OSKIT_PRINCIPAL_H_ */
