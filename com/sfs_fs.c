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

#include "sfs.h"
#include <oskit/c/string.h>

static struct oskit_filesystem_ops fs_ops;
static struct oskit_comsid_ops sid_ops;
static struct oskit_filesystem_secure_ops sfs_ops;

oskit_error_t
sfilesystem_create(oskit_dir_t * root,
		   oskit_security_id_t sid,
		   oskit_services_t * osenv,
		   oskit_filepsid_t * psid,
		   oskit_avc_t * avc,
		   oskit_security_t * security,
		   struct sfilesystem ** out_sfs)
{
	oskit_osenv_log_t *log;
	oskit_osenv_mem_t *mem;
	struct sfilesystem *sfs;


	oskit_services_lookup_first(osenv, &oskit_osenv_log_iid,
				    (void **) &log);
	if (!log)
		return OSKIT_EINVAL;

	oskit_services_lookup_first(osenv, &oskit_osenv_mem_iid,
				    (void **) &mem);
	if (!mem)
		return OSKIT_EINVAL;

	sfs = oskit_osenv_mem_alloc(mem, sizeof(struct sfilesystem), 0, 0);
	if (!sfs)
		return OSKIT_ENOMEM;

	sfs->fsi.ops = &fs_ops;
	sfs->sidi.ops = &sid_ops;
	sfs->sfsi.ops = &sfs_ops;
	sfs->count = 1;
	sfs->sid = sid;
	sfs->mem = mem;
	oskit_osenv_mem_addref(mem);
	sfs->log = log;
	oskit_osenv_log_addref(log);
	sfs->psid = psid;
	oskit_filepsid_addref(psid);
	sfs->avc = avc;
	oskit_avc_addref(avc);
	sfs->security = security;
	oskit_security_addref(security);
	sfs->root = root;
	oskit_dir_addref(root);
	sfs->fs = NULL;
	oskit_dir_getfs(root, &sfs->fs);
	sfs->ino2sf = hashtab_create(mem);
	if (!sfs->ino2sf) {
		if (sfs->fs)
			oskit_filesystem_release(sfs->fs);
		oskit_dir_release(root);
		oskit_security_release(security);
		oskit_avc_release(avc);
		oskit_filepsid_release(psid);
		oskit_osenv_log_release(log);
		oskit_osenv_mem_free(mem, sfs, 0, sizeof(struct sfilesystem));
		oskit_osenv_mem_release(mem);
		return OSKIT_ENOMEM;
	}

	*out_sfs = sfs;

	return 0;
}


/*
 * oskit_filesystem methods
 */

static OSKIT_COMDECL
filesystem_query(oskit_filesystem_t * f,
		 const struct oskit_guid * iid,
		 void **out_ihandle)
{
	struct sfilesystem *sfs;

	sfs = (struct sfilesystem *) f;

	if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
	    memcmp(iid, &oskit_filesystem_iid, sizeof(*iid)) == 0) {
		*out_ihandle = &sfs->fsi;
		++sfs->count;
		return 0;
	}
	if (memcmp(iid, &oskit_comsid_iid, sizeof(*iid)) == 0) {
		*out_ihandle = &sfs->sidi;
		++sfs->count;
		return 0;
	}
	*out_ihandle = NULL;
	return OSKIT_E_NOINTERFACE;
}


static OSKIT_COMDECL_U
filesystem_addref(oskit_filesystem_t * f)
{
	struct sfilesystem *sfs;

	sfs = (struct sfilesystem *) f;

	return ++sfs->count;
}


static OSKIT_COMDECL_U
filesystem_release(oskit_filesystem_t * f)
{
	struct sfilesystem *sfs;
	oskit_osenv_mem_t *mem;
	unsigned        newcount;

	sfs = (struct sfilesystem *) f;

	newcount = --sfs->count;
	if (newcount == 0) {
		if (sfs->fs)
			oskit_filesystem_release(sfs->fs);
		oskit_dir_release(sfs->root);
		oskit_security_release(sfs->security);
		oskit_avc_release(sfs->avc);
		oskit_filepsid_release(sfs->psid);
		oskit_osenv_log_release(sfs->log);
		hashtab_destroy(sfs->ino2sf);
		mem = sfs->mem;
		oskit_osenv_mem_free(mem, sfs, 0, sizeof(struct sfilesystem));
		oskit_osenv_mem_release(mem);
	}
	return newcount;
}


static OSKIT_COMDECL
filesystem_statfs(oskit_filesystem_t * f,
		  oskit_statfs_t * out_stats)
{
	struct sfilesystem *sfs = (struct sfilesystem *) f;
	oskit_security_id_t csid;
	oskit_error_t   rc;


	if (!sfs || !sfs->count || !sfs->fs)
		return OSKIT_E_INVALIDARG;

	CSID(&csid);
	rc = SFS_FS_HAS_PERM(csid, sfs, GETATTR);
	if (rc)
		return rc;

	return oskit_filesystem_statfs(sfs->fs, out_stats);
}


static OSKIT_COMDECL
filesystem_sync(oskit_filesystem_t * f,
		oskit_bool_t wait)
{
	struct sfilesystem *sfs = (struct sfilesystem *) f;

	if (!sfs || !sfs->count || !sfs->fs)
		return OSKIT_E_INVALIDARG;

	return oskit_filesystem_sync(sfs->fs, wait);
}


static OSKIT_COMDECL
filesystem_getroot(oskit_filesystem_t * f,
		   struct oskit_dir ** out_dir)
{
	struct sfilesystem *sfs = (struct sfilesystem *) f;
	struct sfiledir *sdir;
	oskit_error_t   rc;


	if (!sfs || !sfs->count || !sfs->fs)
		return OSKIT_E_INVALIDARG;

	rc = sfiledir_create(sfs, (oskit_file_t *) sfs->root, &sdir);
	if (rc)
		return rc;

	*out_dir = (oskit_dir_t *) & sdir->filei;
	return 0;
}


static OSKIT_COMDECL
filesystem_remount(oskit_filesystem_t * f,
		   oskit_u32_t flags)
{
	struct sfilesystem *sfs = (struct sfilesystem *) f;
	oskit_security_id_t csid;
	oskit_error_t   rc;


	if (!sfs || !sfs->count || !sfs->fs)
		return OSKIT_E_INVALIDARG;

	CSID(&csid);
	rc = SFS_FS_HAS_PERM(csid, sfs, REMOUNT);
	if (rc)
		return rc;

	return oskit_filesystem_remount(sfs->fs, flags);
}


static OSKIT_COMDECL
filesystem_unmount(oskit_filesystem_t * f)
{
	struct sfilesystem *sfs = (struct sfilesystem *) f;
	oskit_security_id_t csid;
	oskit_error_t   rc;


	if (!sfs || !sfs->count || !sfs->fs)
		return OSKIT_E_INVALIDARG;

	CSID(&csid);
	rc = SFS_FS_HAS_PERM(csid, sfs, UNMOUNT);
	if (rc)
		return rc;

	return oskit_filesystem_unmount(sfs->fs);
}


static OSKIT_COMDECL
filesystem_lookupi(oskit_filesystem_t * f,
		   oskit_ino_t ino,
		   oskit_file_t ** out_file)
{
	struct sfilesystem *sfs = (struct sfilesystem *) f;
	oskit_security_id_t csid;
	oskit_error_t   rc;


	if (!sfs || !sfs->count || !sfs->fs)
		return OSKIT_E_INVALIDARG;

	CSID(&csid);
	rc = SFS_FS_HAS_PERM(csid, sfs, LOOKUPI);
	if (rc)
		return rc;

	return oskit_filesystem_lookupi(sfs->fs,ino,out_file);
}


static struct oskit_filesystem_ops fs_ops = {
	filesystem_query,
	filesystem_addref,
	filesystem_release,
	filesystem_statfs,
	filesystem_sync,
	filesystem_getroot,
	filesystem_remount,
	filesystem_unmount,
	filesystem_lookupi,
};



/*
 * oskit_comsid methods
 */

static OSKIT_COMDECL
sid_query(oskit_comsid_t * s,
	  const struct oskit_guid * iid,
	  void **out_ihandle)
{
	struct sfilesystem *sfs;

	sfs = (struct sfilesystem *) ((char *) s -
				      offsetof(struct sfilesystem, sidi));

	return oskit_filesystem_query(&sfs->fsi, iid, out_ihandle);
}


static OSKIT_COMDECL_U
sid_addref(oskit_comsid_t * s)
{
	struct sfilesystem *sfs;

	sfs = (struct sfilesystem *) ((char *) s -
				      offsetof(struct sfilesystem, sidi));

	return oskit_filesystem_addref(&sfs->fsi);
}


static OSKIT_COMDECL_U
sid_release(oskit_comsid_t * s)
{
	struct sfilesystem *sfs;

	sfs = (struct sfilesystem *) ((char *) s -
				      offsetof(struct sfilesystem, sidi));

	return oskit_filesystem_release(&sfs->fsi);
}


static OSKIT_COMDECL
sid_get(oskit_comsid_t * s,
	oskit_security_class_t * outclass,
	oskit_security_id_t * outsid)
{
	struct sfilesystem *sfs;

	sfs = (struct sfilesystem *) ((char *) s -
				      offsetof(struct sfilesystem, sidi));

	*outclass = OSKIT_SECCLASS_FILESYSTEM;
	*outsid = sfs->sid;
	return 0;
}


static OSKIT_COMDECL
sid_set(oskit_comsid_t * s,
	oskit_security_id_t newsid)
{
	struct sfilesystem *sfs;

	sfs = (struct sfilesystem *) ((char *) s -
				      offsetof(struct sfilesystem, sidi));

	return oskit_filepsid_setfs(sfs->psid, newsid, OSKIT_SECSID_NULL);
}


static struct oskit_comsid_ops sid_ops = {
	sid_query,
	sid_addref,
	sid_release,
	sid_get,
	sid_set
};



/*
 * oskit_filesystem_secure methods
 */

static OSKIT_COMDECL
sfs_query(oskit_filesystem_secure_t * s,
	  const struct oskit_guid * iid,
	  void **out_ihandle)
{
	struct sfilesystem *sfs;

	sfs = (struct sfilesystem *) ((char *) s -
				      offsetof(struct sfilesystem, sfsi));

	return oskit_filesystem_query(&sfs->fsi, iid, out_ihandle);
}


static OSKIT_COMDECL_U
sfs_addref(oskit_filesystem_secure_t * s)
{
	struct sfilesystem *sfs;

	sfs = (struct sfilesystem *) ((char *) s -
				      offsetof(struct sfilesystem, sfsi));

	return oskit_filesystem_addref(&sfs->fsi);
}


static OSKIT_COMDECL_U
sfs_release(oskit_filesystem_secure_t * s)
{
	struct sfilesystem *sfs;

	sfs = (struct sfilesystem *) ((char *) s -
				      offsetof(struct sfilesystem, sfsi));

	return oskit_filesystem_release(&sfs->fsi);
}


static OSKIT_COMDECL
sfs_chsidfs(oskit_filesystem_secure_t * s,
	    oskit_security_id_t fs_sid,
	    oskit_security_id_t f_sid)
{
	struct sfilesystem *sfs;
	oskit_security_id_t csid;
	oskit_error_t   rc;

	sfs = (struct sfilesystem *) ((char *) s -
				      offsetof(struct sfilesystem, sfsi));

	if (!sfs || !sfs->count || !sfs->fs)
		return OSKIT_E_INVALIDARG;

	CSID(&csid);
	rc = SFS_FS_HAS_PERM(csid, sfs, RELABELFROM);
	if (rc)
		return rc;
	rc = oskit_avc_has_perm(sfs->avc, sfs->sid, fs_sid,
				OSKIT_SECCLASS_FILESYSTEM,
				OSKIT_PERM_FILESYSTEM__TRANSITION);
	if (rc)
		return rc;
	rc = oskit_avc_has_perm(sfs->avc, csid, fs_sid,
				OSKIT_SECCLASS_FILESYSTEM,
				OSKIT_PERM_FILESYSTEM__RELABELTO);
	if (rc)
		return rc;
	rc = oskit_avc_has_perm(sfs->avc, f_sid, fs_sid,
				OSKIT_SECCLASS_FILESYSTEM,
				OSKIT_PERM_FILESYSTEM__ASSOCIATE);
	if (rc)
		return rc;

	rc = oskit_filepsid_setfs(sfs->psid, fs_sid, f_sid);
	if (rc)
		return rc;

	sfs->sid = fs_sid;
	return 0;
}


static struct oskit_filesystem_secure_ops sfs_ops = {
	sfs_query,
	sfs_addref,
	sfs_release,
	sfs_chsidfs
};
