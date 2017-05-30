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

#ifndef _OSKIT_FS_FILE_SECURE_H
#define _OSKIT_FS_FILE_SECURE_H 1

#include <oskit/com.h>
#include <oskit/fs/file.h>
#include <oskit/flask/flask_types.h>

/*
 * Extensions to oskit_file for Flask.
 * IID 4aa7dfe4-7c74-11cf-b500-08000953adc2
 */
struct oskit_file_secure {
        struct oskit_file_secure_ops *ops;
};
typedef struct oskit_file_secure oskit_file_secure_t;


struct oskit_file_secure_ops {

	/*** COM-specified IUnknown interface operations ***/
	OSKIT_COMDECL	(*query)(oskit_file_secure_t *f,
				 const struct oskit_guid *iid,
				 void **out_ihandle);
	OSKIT_COMDECL_U	(*addref)(oskit_file_secure_t *f);
	OSKIT_COMDECL_U	(*release)(oskit_file_secure_t *f);

	/*** Operations specific to the oskit_file_secure interface ***/

	OSKIT_COMDECL	(*open)(oskit_file_secure_t *f, 
				oskit_oflags_t flags,
				oskit_security_id_t ofsid,
				struct oskit_openfile **out_openfile);

	OSKIT_COMDECL	(*chsid)(oskit_file_secure_t *f,
				 oskit_security_id_t newsid);
};


#define oskit_file_secure_query(f,iid,out_ihandle) ((f)->ops->query((f),(iid),(out_ihandle)))
#define oskit_file_secure_addref(f) ((f)->ops->addref((f)))
#define oskit_file_secure_release(f) ((f)->ops->release((f)))
#define oskit_file_secure_open(f, fl, sid, of) ((f)->ops->open((f),(fl),(sid),(of)))
#define oskit_file_secure_chsid(f, sid) ((f)->ops->chsid((f),(sid)))

/* GUID for oskit_file_secure interface */
extern const struct oskit_guid oskit_file_secure_iid;
#define OSKIT_FILE_SECURE_IID OSKIT_GUID(0x4aa7dfe4, 0x7c74, 0x11cf, \
				0xb5, 0x00, 0x08, 0x00, 0x09, 0x53, 0xad, 0xc2)

#endif
