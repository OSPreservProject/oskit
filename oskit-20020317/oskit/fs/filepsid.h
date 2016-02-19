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

#ifndef _OSKIT_FS_FILEPSID_H
#define _OSKIT_FS_FILEPSID_H 1

#include <oskit/com.h>
#include <oskit/com/services.h>
#include <oskit/fs/file.h>
#include <oskit/flask/flask_types.h>

/*
 * COM file persistent SID interface. 
 * IID 4aa7dfe5-7c74-11cf-b500-08000953adc2
 */
struct oskit_filepsid {
        struct oskit_filepsid_ops *ops;
};
typedef struct oskit_filepsid oskit_filepsid_t;


struct oskit_filepsid_ops {

	/*** COM-specified IUnknown interface operations ***/
	OSKIT_COMDECL	(*query)(oskit_filepsid_t *p,
				 const struct oskit_guid *iid,
				 void **out_ihandle);
	OSKIT_COMDECL_U	(*addref)(oskit_filepsid_t *p);
	OSKIT_COMDECL_U	(*release)(oskit_filepsid_t *p);

	/*** Operations specific to the oskit_filepsid interface ***/

	/*
	 * Get the SID of the specified file from the 
	 * persistent SID mapping.
	 */
	OSKIT_COMDECL	(*get)(oskit_filepsid_t *p,
			       oskit_file_t *file,
			       oskit_security_id_t *outsid);

	/*
	 * Set the SID of the specified file in the 
	 * persistent SID mapping.
	 */
	OSKIT_COMDECL	(*set)(oskit_filepsid_t *p,
			       oskit_file_t *file,
			       oskit_security_id_t newsid);

        /*
	 * Set the SID of the file system and the default
	 * file SID for the file system in the persistent
	 * SID mapping.
	 */
        OSKIT_COMDECL   (*setfs)(oskit_filepsid_t *p,
				 oskit_security_id_t fs_sid,
				 oskit_security_id_t f_sid);
};


#define oskit_filepsid_query(p,iid,out_ihandle) ((p)->ops->query((p),(iid),(out_ihandle)))
#define oskit_filepsid_addref(p) ((p)->ops->addref((p)))
#define oskit_filepsid_release(p) ((p)->ops->release((p)))
#define oskit_filepsid_get(p, f, outsid) ((p)->ops->get((p),(f),(outsid)))
#define oskit_filepsid_set(p, f, newsid) ((p)->ops->set((p),(f),(newsid)))
#define oskit_filepsid_setfs(p, fs_sid, f_sid) ((p)->ops->setfs((p),(fs_sid),(f_sid)))


struct oskit_dir;
struct oskit_security;

oskit_error_t 
oskit_filepsid_create(struct oskit_dir * rootdir,
		      oskit_security_id_t fs_sid,
		      oskit_security_id_t file_sid,
		      oskit_services_t *osenv,
		      struct oskit_security * security,
		      oskit_security_id_t *out_fs_sid,
		      oskit_filepsid_t ** out_filepsid);


/* GUID for oskit_filepsid interface */
extern const struct oskit_guid oskit_filepsid_iid;
#define OSKIT_FILEPSID_IID OSKIT_GUID(0x4aa7dfe5, 0x7c74, 0x11cf, \
				0xb5, 0x00, 0x08, 0x00, 0x09, 0x53, 0xad, 0xc2)

#endif
