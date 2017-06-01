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

#ifndef _OSKIT_FS_DIR_SECURE_H
#define _OSKIT_FS_DIR_SECURE_H 1

#include <oskit/com.h>
#include <oskit/fs/file.h>
#include <oskit/fs/dir.h>
#include <oskit/flask/flask_types.h>

/*
 * Extensions to oskit_dir for Flask.
 * IID 4aa7dfe3-7c74-11cf-b500-08000953adc2
 */
struct oskit_dir_secure {
        struct oskit_dir_secure_ops *ops;
};
typedef struct oskit_dir_secure oskit_dir_secure_t;


struct oskit_dir_secure_ops {

	/*** COM-specified IUnknown interface operations ***/
	OSKIT_COMDECL	(*query)(oskit_dir_secure_t *d,
				 const struct oskit_guid *iid,
				 void **out_ihandle);
	OSKIT_COMDECL_U	(*addref)(oskit_dir_secure_t *d);
	OSKIT_COMDECL_U	(*release)(oskit_dir_secure_t *d);

	/*** Operations specific to the oskit_dir_secure interface ***/

	OSKIT_COMDECL	(*create)(oskit_dir_secure_t *d, 
				  const char *name,
				  oskit_bool_t excl, 
				  oskit_mode_t mode,
				  oskit_security_id_t sid,
				  oskit_file_t **out_file);

	OSKIT_COMDECL	(*mkdir)(oskit_dir_secure_t *d, 
				 const char *name,
				 oskit_mode_t mode,
				 oskit_security_id_t sid);
};


#define oskit_dir_secure_query(d,iid,out_ihandle) ((d)->ops->query((d),(iid),(out_ihandle)))
#define oskit_dir_secure_addref(d) ((d)->ops->addref((d)))
#define oskit_dir_secure_release(d) ((d)->ops->release((d)))
#define oskit_dir_secure_create(d, n, x, m, s, of) ((d)->ops->create((d),(n),(x),(m),(s),(of)))
#define oskit_dir_secure_mkdir(d, n, m, s) ((d)->ops->mkdir((d),(n),(m),(s)))

/* GUID for oskit_dir_secure interface */
extern const struct oskit_guid oskit_dir_secure_iid;
#define OSKIT_DIR_SECURE_IID OSKIT_GUID(0x4aa7dfe3, 0x7c74, 0x11cf, \
				0xb5, 0x00, 0x08, 0x00, 0x09, 0x53, 0xad, 0xc2)

#endif
