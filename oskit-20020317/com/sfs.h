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

#ifndef _OSKIT__COM_SFS_H_
#define _OSKIT__COM_SFS_H_

#include <oskit/com.h>
#include <oskit/com/services.h>
#include <oskit/principal.h>
#include <oskit/dev/osenv_mem.h>
#include <oskit/dev/osenv_log.h>
#include <oskit/io/absio.h>
#include <oskit/fs/filesystem.h>
#include <oskit/fs/file.h>
#include <oskit/fs/dir.h>
#include <oskit/fs/openfile.h>
#include <oskit/fs/fs.h>
#include <oskit/flask/security.h>
#include <oskit/flask/avc.h>
#include <oskit/comsid.h>
#include <oskit/fs/filepsid.h>
#include <oskit/fs/filesystem_secure.h>
#include <oskit/fs/file_secure.h>
#include <oskit/fs/dir_secure.h>
#include "sfs_new_hashtab.h"


struct sfilesystem
{
    oskit_filesystem_t	fsi;	/* COM filesystem interface */
    oskit_comsid_t	sidi;	/* COM sid interface */
    oskit_filesystem_secure_t sfsi; /* Flask extensions to oskit_filesystem */
    unsigned		count;	/* reference count */
    oskit_security_id_t	sid;	/* fs security identifier */
    oskit_osenv_mem_t	*mem;	/* memory allocation */
    oskit_osenv_log_t	*log;	/* logging */
    oskit_filepsid_t	*psid;	/* persistent SID mapping */
    oskit_avc_t		*avc;	/* access vector cache */
    oskit_security_t	*security; /* security server */
    oskit_dir_t		*root;	/* underlying root directory */
    oskit_filesystem_t   *fs;	/* underlying file system */
    hashtab_t 		ino2sf;	/* ino -> sfiledir */
};


struct sfiledir
{
    oskit_file_t filei;		/* COM file/dir interface */
    oskit_absio_t absioi;	/* COM absolute I/O interface */
    oskit_comsid_t sidi;	/* COM sid interface */
    oskit_file_secure_t sfilei;	/* Flask extensions to oskit_file */
    oskit_dir_secure_t sdiri;	/* Flask extensions to oskit_dir */
    unsigned count;		/* reference count */
    oskit_security_id_t sid;	/* security identifier */
    oskit_security_class_t sclass; /* security class */
    oskit_avc_entry_ref_t avcr;	/* permissions to this file */
    struct sfilesystem *sfs;	/* containing file system */
    oskit_file_t *file;		/* underlying file interface */    
    oskit_absio_t *absio;	/* underlying absio interface */
    struct sopenfile *sopenfile_list;
				/* openfiles refering to this sfiledir */
};


struct sopenfile
{
    oskit_openfile_t ofilei;	/* COM open file interface */
    oskit_absio_t absioi;	/* COM absolute I/O interface */
    oskit_comsid_t sidi;	/* COM sid interface */ 
    unsigned count;		/* reference count */
    oskit_oflags_t flags;	/* open flags */	
    oskit_security_id_t sid;	/* security identifier */
    oskit_avc_entry_ref_t avcr; /* permissions to this openfile */
    oskit_avc_entry_ref_t file_avcr; /* permissions to associated file */
    struct sfiledir *sfile;	/* associated file */
    oskit_openfile_t *ofile;	/* underlying open file object */
    oskit_absio_t *absio;	/* underlying absio interface */
    struct sopenfile *next; 	/* forward link to reference to same file */
    struct sopenfile *prev;	/* reverse link to reference to same file */
};

#define sfilesystem_create oskit_com_sfs_sfilesystem_create
#define sfiledir_create oskit_com_sfs_sfiledir_create
#define sopenfile_create oskit_com_sfs_sopenfile_create

oskit_error_t sfilesystem_create(oskit_dir_t *root, 
				 oskit_security_id_t sid,
				 oskit_services_t *osenv,
				 oskit_filepsid_t *psid,
				 oskit_avc_t *avc,
				 oskit_security_t *security,
				 struct sfilesystem **out_sfs);

oskit_error_t sfiledir_create(struct sfilesystem *sfs,
			     oskit_file_t *file,
			     struct sfiledir **out_sfiledir);	

oskit_error_t sopenfile_create(struct sfiledir *sfile, 
			      oskit_openfile_t *ofile,
			      oskit_oflags_t flags,
			      oskit_security_id_t sid,
			      struct sopenfile **out_sofile);    


#ifndef offsetof
#define	offsetof(type, member)	((oskit_size_t)(&((type *)0)->member))
#endif


#define SFS_FS_HAS_PERM(_csid, _sfs, _perm) \
	oskit_avc_has_perm((_sfs)->avc, (_csid), (_sfs)->sid, \
			   OSKIT_SECCLASS_FILESYSTEM, \
			   OSKIT_PERM_FILESYSTEM__##_perm)

#define SFS_FILE_HAS_PERM(_csid, _sfile, _perm) \
	oskit_avc_has_perm_ref((_sfile)->sfs->avc, \
			       (_csid), (_sfile)->sid, (_sfile)->sclass, \
			       OSKIT_PERM_FILE__##_perm, \
			       &((_sfile)->avcr))

#define SFS_DIR_HAS_PERM(_csid, _sfile, _perm) \
	oskit_avc_has_perm_ref((_sfile)->sfs->avc, \
			       (_csid), (_sfile)->sid, (_sfile)->sclass, \
			       OSKIT_PERM_DIR__##_perm, \
			       &((_sfile)->avcr))

#define SFS_DIR_HAS_PERM2(_csid, _sfile, _perm1, _perm2) \
	oskit_avc_has_perm_ref((_sfile)->sfs->avc, \
			       (_csid), (_sfile)->sid, (_sfile)->sclass, \
			       OSKIT_PERM_DIR__##_perm1 | \
			       OSKIT_PERM_DIR__##_perm2 , \
			       &((_sfile)->avcr))

#define SFS_FD_HAS_PERM(_csid, _sopenfile, _perm) \
	oskit_avc_has_perm_ref((_sopenfile)->sfile->sfs->avc, \
			   (_csid), (_sopenfile)->sid, OSKIT_SECCLASS_FD, \
			   OSKIT_PERM_FD__##_perm, \
			   &((_sopenfile)->avcr))

#define SFS_FD_FILE_HAS_PERM(_csid, _sopenfile, _perm) \
	oskit_avc_has_perm_ref((_sopenfile)->sfile->sfs->avc, \
			       (_csid), \
			       (_sopenfile)->sfile->sid, \
			       (_sopenfile)->sfile->sclass, \
			       OSKIT_PERM_FILE__##_perm, \
			       &((_sopenfile)->file_avcr))

#endif _OSKIT__COM_SFS_H_

