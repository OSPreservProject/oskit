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

#include <oskit/c/string.h>
#include <oskit/fs/soa.h>
#include "sfs.h"

static struct oskit_file_ops file_ops;
static struct oskit_absio_ops file_absio_ops;
static struct oskit_dir_ops dir_ops;
static struct oskit_comsid_ops sid_ops;
static struct oskit_file_secure_ops sfile_ops;
static struct oskit_dir_secure_ops sdir_ops;


static oskit_security_class_t file_mode_to_security_class(int mode)
{
	switch (mode & OSKIT_S_IFMT) {
	case OSKIT_S_IFSOCK:
		return OSKIT_SECCLASS_SOCK_FILE;
	case OSKIT_S_IFLNK:
		return OSKIT_SECCLASS_LNK_FILE;
	case OSKIT_S_IFREG:
		return OSKIT_SECCLASS_FILE;
	case OSKIT_S_IFBLK:
		return OSKIT_SECCLASS_BLK_FILE;
	case OSKIT_S_IFDIR:
		return OSKIT_SECCLASS_DIR;
	case OSKIT_S_IFCHR:
		return OSKIT_SECCLASS_CHR_FILE;
	case OSKIT_S_IFIFO:
		return OSKIT_SECCLASS_FIFO_FILE;

	}

	return OSKIT_SECCLASS_NULL;
}


oskit_error_t
sfiledir_create(struct sfilesystem * sfs,
		oskit_file_t * file,
		struct sfiledir ** out_sfiledir)
{
	oskit_dir_t    *d;
	struct sfiledir *sfiledir;
	oskit_stat_t    stats;
	oskit_error_t   rc;


	rc = oskit_file_stat(file, &stats);
	if (rc)
		return rc;

	sfiledir = (struct sfiledir *)
		hashtab_search(sfs->ino2sf, (hashtab_key_t) stats.ino);

	if (sfiledir) {
		oskit_file_addref(&sfiledir->filei);
		*out_sfiledir = sfiledir;
		return 0;
	}

	sfiledir = oskit_osenv_mem_alloc(sfs->mem,sizeof(struct sfiledir),0,0);
	if (!sfiledir)
		return OSKIT_ENOMEM;

	sfiledir->sclass = file_mode_to_security_class(stats.mode);
	if (!sfiledir->sclass) {
		rc = oskit_file_query(file, &oskit_dir_iid, (void **) &d);
		if (rc) {
			sfiledir->sclass = OSKIT_SECCLASS_FILE;
		}
		else {
			oskit_dir_release(d);
			sfiledir->sclass = OSKIT_SECCLASS_DIR;
		}
	}
	OSKIT_AVC_ENTRY_REF_INIT(&sfiledir->avcr);

	if (sfiledir->sclass != OSKIT_SECCLASS_DIR) {
		sfiledir->filei.ops = &file_ops;
	} else {
		sfiledir->filei.ops = (struct oskit_file_ops *) & dir_ops;
		sfiledir->sdiri.ops = &sdir_ops;
	}

	sfiledir->sfilei.ops = &sfile_ops;
	sfiledir->absioi.ops = &file_absio_ops;
	sfiledir->sidi.ops = &sid_ops;
	sfiledir->count = 1;

	rc = oskit_filepsid_get(sfs->psid, file, &sfiledir->sid);
	if (rc) {
		oskit_osenv_mem_free(sfs->mem, sfiledir, 0,
				     sizeof(struct sfiledir));
		return rc;
	}
	rc = hashtab_insert(sfs->ino2sf,
			    (hashtab_key_t) stats.ino,
			    (hashtab_datum_t) & sfiledir->filei);
	if (rc) {
		oskit_osenv_mem_free(sfs->mem, sfiledir, 0,
				     sizeof(struct sfiledir));
		return rc;
	}
	sfiledir->sfs = sfs;
	oskit_filesystem_addref(&sfs->fsi);

	sfiledir->file = file;
	oskit_file_addref(file);

	rc = oskit_file_query(file, &oskit_absio_iid, (void **) &sfiledir->absio);
	if (rc)
		sfiledir->absio = NULL;

	sfiledir->sopenfile_list = NULL;

	*out_sfiledir = sfiledir;
	return 0;
}


/*
 * oskit_file methods
 */

static OSKIT_COMDECL
file_query(oskit_file_t * f,
	   const struct oskit_guid * iid,
	   void **out_ihandle)
{
	struct sfiledir *sfile;

	sfile = (struct sfiledir *) f;
	if (!sfile || !sfile->count)
		return OSKIT_EINVAL;

	if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
	    memcmp(iid, &oskit_posixio_iid, sizeof(*iid)) == 0 ||
	    memcmp(iid, &oskit_file_iid, sizeof(*iid)) == 0) {
		*out_ihandle = &sfile->filei;
		++sfile->count;
		return 0;
	}
	if (memcmp(iid, &oskit_comsid_iid, sizeof(*iid)) == 0) {
		*out_ihandle = &sfile->sidi;
		++sfile->count;
		return 0;
	}
	if (memcmp(iid, &oskit_file_secure_iid, sizeof(*iid)) == 0) {
		*out_ihandle = &sfile->sfilei;
		++sfile->count;
		return 0;
	}
	if (sfile->absio && memcmp(iid, &oskit_absio_iid, sizeof(*iid)) == 0) {
		*out_ihandle = &sfile->absioi;
		++sfile->count;
		return 0;
	}
	*out_ihandle = NULL;
	return OSKIT_E_NOINTERFACE;
}


static OSKIT_COMDECL_U
file_addref(oskit_file_t * f)
{
	struct sfiledir *sfile;

	sfile = (struct sfiledir *) f;
	if (!sfile || !sfile->count)
		return OSKIT_EINVAL;

	return ++sfile->count;
}


static OSKIT_COMDECL_U
file_release(oskit_file_t * f)
{
	struct sfiledir *sfile;
	oskit_stat_t    stats;
	oskit_error_t   rc;
	unsigned        newcount;

	sfile = (struct sfiledir *) f;
	if (!sfile || !sfile->count)
		return OSKIT_EINVAL;

	newcount = --sfile->count;
	if (newcount == 0) {
		rc = oskit_file_stat(sfile->file, &stats);
		if (!rc) {
			(void) hashtab_remove(sfile->sfs->ino2sf,
					      (hashtab_key_t) stats.ino);

		}
		oskit_filesystem_release(&sfile->sfs->fsi);
		oskit_file_release(sfile->file);
		if (sfile->absio)
			oskit_absio_release(sfile->absio);
		oskit_osenv_mem_free(sfile->sfs->mem, sfile, 0,
				     sizeof(struct sfiledir));
	}
	return newcount;
}


static OSKIT_COMDECL
file_stat(oskit_file_t * f,
	  struct oskit_stat * out_stats)
{
	struct sfiledir *sfile;
	oskit_security_id_t   csid;
	oskit_error_t   rc;

	sfile = (struct sfiledir *) f;

	if (!sfile || !sfile->count)
		return OSKIT_E_INVALIDARG;

	CSID(&csid);
	rc = SFS_FILE_HAS_PERM(csid, sfile, GETATTR);
	if (rc)
		return rc;

	return oskit_file_stat(sfile->file, out_stats);
}


static OSKIT_COMDECL
file_setstat(oskit_file_t * f, oskit_u32_t mask,
	     const struct oskit_stat * stats)
{
	struct sfiledir *sfile;
	oskit_error_t   rc;
	oskit_security_id_t   csid;


	sfile = (struct sfiledir *) f;

	if (!sfile || !sfile->count)
		return OSKIT_E_INVALIDARG;

	CSID(&csid);
	rc = SFS_FILE_HAS_PERM(csid, sfile, SETATTR);
	if (rc)
		return rc;

	return oskit_file_setstat(sfile->file, mask, stats);
}


static OSKIT_COMDECL
file_pathconf(oskit_file_t * f, oskit_s32_t option,
	      oskit_s32_t * out_val)
{
	struct sfiledir *sfile;
	oskit_security_id_t   csid;
	oskit_error_t   rc;

	sfile = (struct sfiledir *) f;

	if (!sfile || !sfile->count)
		return OSKIT_E_INVALIDARG;

	CSID(&csid);
	rc = SFS_FILE_HAS_PERM(csid, sfile, PATHCONF);
	if (rc)
		return rc;

	return oskit_file_pathconf(sfile->file, option, out_val);
}


static OSKIT_COMDECL
file_sync(oskit_file_t * f, oskit_bool_t wait)
{
	struct sfiledir *sfile;

	sfile = (struct sfiledir *) f;

	if (!sfile || !sfile->count)
		return OSKIT_E_INVALIDARG;

	return oskit_file_sync(sfile->file, wait);
}


static OSKIT_COMDECL
file_access(oskit_file_t * f, oskit_amode_t mask)
{
	struct sfiledir *sfile;
	oskit_security_id_t   csid;
	oskit_error_t   rc;

	sfile = (struct sfiledir *) f;

	if (!sfile || !sfile->count)
		return OSKIT_E_INVALIDARG;

	CSID(&csid);
	rc = SFS_FILE_HAS_PERM(csid, sfile, ACCESS);
	if (rc)
		return rc;

	return oskit_file_access(sfile->file, mask);
}


static OSKIT_COMDECL
file_readlink(oskit_file_t * f,
	      char *buf, oskit_u32_t len,
	      oskit_u32_t * out_actual)
{
	struct sfiledir *sfile;
	oskit_error_t   rc;
	oskit_security_id_t   csid;

	sfile = (struct sfiledir *) f;
	if (!sfile || !sfile->count)
		return OSKIT_E_INVALIDARG;

	if (f->ops != &file_ops)
		return OSKIT_E_NOTIMPL;

	if (sfile->sclass != OSKIT_SECCLASS_LNK_FILE)
		return OSKIT_E_NOTIMPL;

	CSID(&csid);
	rc = SFS_FILE_HAS_PERM(csid, sfile, READ);
	if (rc)
		return rc;

	return oskit_file_readlink(sfile->file, buf, len, out_actual);
}




static OSKIT_COMDECL
file_open(oskit_file_t * f, oskit_oflags_t iflags,
	  struct oskit_openfile ** out_openfile)
{
	struct sfiledir *sfile;
	oskit_error_t   rc;
	oskit_security_id_t   csid;

	sfile = (struct sfiledir *) f;
	if (!sfile || !sfile->count)
		return OSKIT_E_INVALIDARG;

	CSID(&csid);
	rc = oskit_file_secure_open(&sfile->sfilei, iflags, csid, out_openfile);

	return rc;
}


static OSKIT_COMDECL
file_getfs(oskit_file_t * f, struct oskit_filesystem ** out_fs)
{
	struct sfiledir *sfile;

	sfile = (struct sfiledir *) f;

	if (!sfile || !sfile->count)
		return OSKIT_E_INVALIDARG;

	oskit_filesystem_addref(&sfile->sfs->fsi);
	*out_fs = &sfile->sfs->fsi;

	return 0;
}


static struct oskit_file_ops file_ops = {
	file_query,
	file_addref,
	file_release,
	file_stat,
	file_setstat,
	file_pathconf,
	file_sync,
	file_sync,
	file_access,
	file_readlink,
	file_open,
	file_getfs
};




/*
 * methods for absio interface of files
 */

static OSKIT_COMDECL
afile_query(oskit_absio_t * io,
	    const struct oskit_guid * iid,
	    void **out_ihandle)
{
	struct sfiledir *sfile;

	if (!io || io->ops != &file_absio_ops)
		return OSKIT_E_INVALIDARG;

	sfile = (struct sfiledir *) ((char *) io - offsetof(struct sfiledir, absioi));
	if (!sfile->count)
		return OSKIT_E_INVALIDARG;

	/* may be file_query or dir_query */
	return oskit_file_query(&sfile->filei, iid, out_ihandle);
}

static OSKIT_COMDECL_U
afile_addref(oskit_absio_t * io)
{
	struct sfiledir *sfile;

	if (!io || io->ops != &file_absio_ops)
		return OSKIT_E_INVALIDARG;

	sfile = (struct sfiledir *) ((char *) io - offsetof(struct sfiledir, absioi));

	return file_addref(&sfile->filei);
}

static OSKIT_COMDECL_U
afile_release(oskit_absio_t * io)
{
	struct sfiledir *sfile;

	if (!io || io->ops != &file_absio_ops)
		return OSKIT_E_INVALIDARG;

	sfile = (struct sfiledir *) ((char *) io - offsetof(struct sfiledir, absioi));

	return file_release(&sfile->filei);
}


static OSKIT_COMDECL
afile_read(oskit_absio_t * io, void *buf,
	   oskit_off_t offset, oskit_size_t amount,
	   oskit_size_t * out_actual)
{
	struct sfiledir *sfile;
	oskit_security_id_t   csid;
	oskit_error_t   rc;

	if (!io || io->ops != &file_absio_ops)
		return OSKIT_E_INVALIDARG;

	sfile = (struct sfiledir *) ((char *) io - offsetof(struct sfiledir, absioi));
	if (!sfile->count)
		return OSKIT_E_INVALIDARG;

	CSID(&csid);
	rc = SFS_FILE_HAS_PERM(csid, sfile, READ);
	if (rc)
		return rc;

	return oskit_absio_read(sfile->absio, buf, offset, amount, out_actual);
}


static OSKIT_COMDECL
afile_write(oskit_absio_t * io, const void *buf,
	    oskit_off_t offset, oskit_size_t amount,
	    oskit_size_t * out_actual)
{
	struct sfiledir *sfile;
	oskit_security_id_t   csid;
	oskit_error_t   rc;

	if (!io || io->ops != &file_absio_ops)
		return OSKIT_E_INVALIDARG;

	sfile = (struct sfiledir *) ((char *) io - offsetof(struct sfiledir, absioi));
	if (!sfile->count)
		return OSKIT_E_INVALIDARG;

	if (sfile->filei.ops != &file_ops)
		return OSKIT_EISDIR;

	CSID(&csid);
	rc = SFS_FILE_HAS_PERM(csid, sfile, WRITE);
	if (rc)
		return rc;

	return oskit_absio_write(sfile->absio, buf, offset, amount, out_actual);
}


static OSKIT_COMDECL
afile_get_size(oskit_absio_t * io, oskit_off_t * out_size)
{
	struct sfiledir *sfile;
	oskit_security_id_t   csid;
	oskit_error_t   rc;

	if (!io || io->ops != &file_absio_ops)
		return OSKIT_E_INVALIDARG;

	sfile = (struct sfiledir *) ((char *) io - offsetof(struct sfiledir, absioi));
	if (!sfile->count)
		return OSKIT_E_INVALIDARG;

	CSID(&csid);
	rc = SFS_FILE_HAS_PERM(csid, sfile, GETATTR);
	if (rc)
		return rc;

	return oskit_absio_getsize(sfile->absio, out_size);
}


static OSKIT_COMDECL
afile_set_size(oskit_absio_t * io, oskit_off_t new_size)
{
	struct sfiledir *sfile;
	oskit_security_id_t   csid;
	oskit_error_t   rc;

	if (!io || io->ops != &file_absio_ops)
		return OSKIT_E_INVALIDARG;

	sfile = (struct sfiledir *) ((char *) io - offsetof(struct sfiledir, absioi));
	if (!sfile->count)
		return OSKIT_E_INVALIDARG;

	CSID(&csid);
	rc = SFS_FILE_HAS_PERM(csid, sfile, SETATTR);
	if (rc)
		return rc;

	return oskit_absio_setsize(sfile->absio, new_size);
}


static struct oskit_absio_ops file_absio_ops = {
	afile_query,
	afile_addref,
	afile_release,
	(void *) 0,		/* slot reserved for getblocksize */
	afile_read,
	afile_write,
	afile_get_size,
	afile_set_size
};




/*
 * oskit_dir methods; we inherit all the file methods above except query
 */

static OSKIT_COMDECL
dir_query(oskit_dir_t * d,
	  const struct oskit_guid * iid,
	  void **out_ihandle)
{
	struct sfiledir *sdir;

	sdir = (struct sfiledir *) d;

	if (memcmp(iid, &oskit_dir_iid, sizeof(*iid)) == 0) {
		*out_ihandle = &sdir->filei;
		++sdir->count;
		return 0;
	}
	if (memcmp(iid, &oskit_dir_secure_iid, sizeof(*iid)) == 0) {
		*out_ihandle = &sdir->sdiri;
		++sdir->count;
		return 0;
	}
	return file_query((oskit_file_t *) d, iid, out_ihandle);
}


static inline oskit_error_t
dir_lookup_helper(struct sfiledir * sdir,
		  const char *name,
		  struct sfiledir ** out_sfile)
{
	oskit_file_t   *file;
	oskit_error_t   rc;

	rc = oskit_dir_lookup((oskit_dir_t *) sdir->file, name, &file);
	if (rc)
		return rc;

	rc = sfiledir_create(sdir->sfs, file, out_sfile);
	oskit_file_release(file);
	return rc;
}


static OSKIT_COMDECL
dir_lookup(oskit_dir_t * d, const char *name,
	   oskit_file_t ** out_file)
{
	oskit_security_id_t   csid;
	struct sfiledir *sdir, *sfile;
	oskit_error_t   rc;

	sdir = (struct sfiledir *) d;
	if (!sdir || !sdir->count)
		return OSKIT_EINVAL;

	CSID(&csid);
	rc = SFS_DIR_HAS_PERM(csid, sdir, SEARCH);
	if (rc)
		return rc;

	rc = dir_lookup_helper(sdir, name, &sfile);
	if (rc)
		return rc;

	*out_file = &sfile->filei;
	return 0;
}


static OSKIT_COMDECL
dir_create(oskit_dir_t * d, const char *name,
	   oskit_bool_t excl, oskit_mode_t mode,
	   oskit_file_t ** out_file)
{
	struct sfiledir *sdir;
	oskit_security_id_t   csid, fsid;
	oskit_error_t   rc;


	sdir = (struct sfiledir *) d;

	CSID(&csid);

	rc = oskit_security_transition_sid(sdir->sfs->security,
					   csid, sdir->sid,
					   OSKIT_SECCLASS_FILE,
					   &fsid);
	if (rc)
		return rc;

	return oskit_dir_secure_create(&sdir->sdiri, name, excl, mode, fsid, out_file);
}


static OSKIT_COMDECL
dir_link(oskit_dir_t * d, char *name,
	 oskit_file_t * f)
{
	struct sfiledir *sdir, *sfile;
	oskit_error_t   rc;
	oskit_security_id_t   csid;

	sdir = (struct sfiledir *) d;
	if (f->ops != &file_ops && f->ops != (struct oskit_file_ops *) & dir_ops)
		return OSKIT_EXDEV;

	sfile = (struct sfiledir *) f;
	if (!sfile || !sfile->count)
		return OSKIT_EINVAL;

	CSID(&csid);
	rc = SFS_DIR_HAS_PERM2(csid, sdir, SEARCH, ADD_NAME);
	if (rc)
		return rc;

	rc = SFS_FILE_HAS_PERM(csid, sfile, LINK);
	if (rc)
		return rc;

	return oskit_dir_link((oskit_dir_t *) sdir->file, name, sfile->file);
}


static OSKIT_COMDECL
dir_unlink(oskit_dir_t * d, const char *name)
{
	struct sfiledir *sdir, *sfile;
	oskit_stat_t    stats;
	oskit_security_id_t   csid;
	oskit_error_t   rc;

	sdir = (struct sfiledir *) d;

	CSID(&csid);

	rc = SFS_DIR_HAS_PERM2(csid, sdir, SEARCH, REMOVE_NAME);
	if (rc)
		return rc;
	rc = dir_lookup_helper(sdir, name, &sfile);
	if (rc)
		return rc;

	rc = SFS_FILE_HAS_PERM(csid, sfile, UNLINK);
	if (rc) {
		oskit_file_release(&sfile->filei);
		return rc;
	}
	rc = oskit_file_stat(sfile->file, &stats);
	oskit_file_release(&sfile->filei);
	if (rc)
		return rc;

	return oskit_dir_unlink((oskit_dir_t *) sdir->file, name);
}


static OSKIT_COMDECL
dir_rename(oskit_dir_t * old_d, char *old_name,
	   oskit_dir_t * new_d, char *new_name)
{
	struct sfiledir *sdirf, *sdirt, *sfilef, *sfilet;
	oskit_security_id_t   csid;
	oskit_error_t   rc;

	sdirf = (struct sfiledir *) old_d;
	if (new_d->ops != &dir_ops)
		return OSKIT_EXDEV;

	sdirt = (struct sfiledir *) new_d;

	CSID(&csid);
	rc = SFS_DIR_HAS_PERM2(csid, sdirf, SEARCH, REMOVE_NAME);
	if (!rc)
		rc = SFS_DIR_HAS_PERM2(csid, sdirt, SEARCH, ADD_NAME);
	if (rc)
		return rc;

	rc = dir_lookup_helper(sdirf, old_name, &sfilef);
	if (rc)
		return rc;

	rc = SFS_FILE_HAS_PERM(csid, sfilef, RENAME);
	if (rc)
		return rc;

	if (sdirf != sdirt && sfilef->sclass == OSKIT_SECCLASS_DIR) {
		rc = SFS_DIR_HAS_PERM(csid, sfilef, REPARENT);
		if (rc) {
			oskit_file_release(&sfilef->filei);
			return rc;
		}
	}
	oskit_file_release(&sfilef->filei);

	rc = dir_lookup_helper(sdirt, new_name, &sfilet);
	if (!rc) {
		rc = SFS_DIR_HAS_PERM(csid, sdirt, REMOVE_NAME);
		if (rc) {
			oskit_file_release(&sfilet->filei);
			return rc;
		}
		if (sfilet->sclass == OSKIT_SECCLASS_DIR)
			rc = SFS_DIR_HAS_PERM(csid, sfilet, RMDIR);
		else
			rc = SFS_FILE_HAS_PERM(csid, sfilet, UNLINK);
		if (rc) {
			oskit_file_release(&sfilet->filei);
			return rc;
		}
		oskit_file_release(&sfilet->filei);
	} else if (rc != OSKIT_ENOENT)
		return rc;

	return oskit_dir_rename((oskit_dir_t *) sdirf->file,
				old_name,
				(oskit_dir_t *) sdirt->file,
				new_name);
}


static inline oskit_error_t
file_create_checks(oskit_security_id_t csid,
		   struct sfiledir * sdir,
		   oskit_security_class_t tclass,
		   oskit_security_id_t new_fsid)
{
	oskit_error_t   rc;

	rc = SFS_DIR_HAS_PERM2(csid, sdir, SEARCH, ADD_NAME);
	if (rc)
		return rc;

	rc = oskit_avc_has_perm(sdir->sfs->avc, csid, new_fsid,
				tclass, OSKIT_PERM_FILE__CREATE);
	if (rc)
		return rc;

	return SFS_FS_HAS_PERM(new_fsid, sdir->sfs, ASSOCIATE);
}


#define DIR_MKCALL(call, d, tclass, fsid, name, args...) { \
    struct sfiledir *sdir; \
    oskit_security_id_t csid; \
    oskit_file_t *file; \
    oskit_bool_t rfsid = FALSE; \
    oskit_error_t rc; \
\
\
    sdir = (struct sfiledir*)(d); \
\
    CSID(&csid); \
    if (!fsid) \
    { \
	rc = oskit_security_transition_sid(sdir->sfs->security, \
					   csid, sdir->sid,\
					   tclass, \
					   &fsid); \
        if (rc) \
	        return rc; \
        rfsid = TRUE; \
    } \
\
    rc = file_create_checks(csid, sdir, (tclass), (fsid)); \
     \
    if (rc) \
	    return rc; \
\
    rc = oskit_dir_##call##((oskit_dir_t*)sdir->file,(name),##args); \
    if (rc) \
	    return rc; \
\
    rc = oskit_dir_lookup((oskit_dir_t*)sdir->file,(name),&file); \
    if (rc) \
	    return rc; \
\
    rc = oskit_filepsid_set(sdir->sfs->psid, file, (fsid)); \
    oskit_file_release(file); \
    if (rc) \
    { \
        if (tclass == OSKIT_SECCLASS_DIR) \
	      oskit_dir_rmdir((oskit_dir_t*)sdir->file,(name)); \
        else \
	      oskit_dir_unlink((oskit_dir_t*)sdir->file,(name)); \
    } \
    return rc; \
}

static OSKIT_COMDECL
dir_mkdir(oskit_dir_t * d, const char *name,
	  oskit_mode_t mode)
{
	oskit_security_id_t   fsid = OSKIT_SECSID_NULL;
	DIR_MKCALL(mkdir, d, OSKIT_SECCLASS_DIR, fsid, name, mode);
}


static OSKIT_COMDECL
dir_rmdir(oskit_dir_t * d, const char *name)
{
	struct sfiledir *sdir, *sfile;
	oskit_security_id_t   csid;
	oskit_error_t   rc;

	sdir = (struct sfiledir *) d;

	CSID(&csid);

	rc = SFS_DIR_HAS_PERM2(csid, sdir, SEARCH, REMOVE_NAME);
	if (rc)
		return rc;

	rc = dir_lookup_helper(sdir, name, &sfile);
	if (rc)
		return rc;

	rc = SFS_DIR_HAS_PERM(csid, sfile, RMDIR);
	if (rc) {
		oskit_file_release(&sfile->filei);
		return rc;
	}
	oskit_file_release(&sfile->filei);

	return oskit_dir_rmdir((oskit_dir_t *) sdir->file, name);
}


static OSKIT_COMDECL
dir_getdirentries(oskit_dir_t * d, oskit_u32_t * inout_ofs,
		  oskit_u32_t nentries,
		  struct oskit_dirents ** out_dirents)
{
	struct sfiledir *sdir;
	oskit_security_id_t   csid;
	oskit_error_t   rc = 0;

	sdir = (struct sfiledir *) d;

	CSID(&csid);
	rc = SFS_FILE_HAS_PERM(csid, sdir, READ);
	if (rc)
		return rc;

	return oskit_dir_getdirentries((oskit_dir_t *) sdir->file, inout_ofs,
				       nentries, out_dirents);
}


static OSKIT_COMDECL
dir_mknod(oskit_dir_t * d, char *name,
	  oskit_mode_t mode, oskit_dev_t dev)
{
	oskit_security_id_t   fsid = OSKIT_SECSID_NULL;
	oskit_security_class_t tclass = file_mode_to_security_class(mode);
	DIR_MKCALL(mknod, d, tclass, fsid, name, mode, dev);
}


static OSKIT_COMDECL
dir_symlink(oskit_dir_t * d, char *link_name,
	    char *dest_name)
{
	oskit_security_id_t   fsid = OSKIT_SECSID_NULL;
	DIR_MKCALL(symlink, d, OSKIT_SECCLASS_LNK_FILE, fsid, link_name, dest_name);
}


static OSKIT_COMDECL
dir_reparent(oskit_dir_t * d, oskit_dir_t * parent,
	     oskit_dir_t ** out_dir)
{
	return OSKIT_E_NOTIMPL;
}


static struct oskit_dir_ops dir_ops = {
	dir_query,
	file_addref,
	file_release,
	file_stat,
	file_setstat,
	file_pathconf,
	file_sync,
	file_sync,
	file_access,
	file_readlink,
	file_open,
	file_getfs,
	dir_lookup,
	dir_create,
	dir_link,
	dir_unlink,
	dir_rename,
	dir_mkdir,
	dir_rmdir,
	dir_getdirentries,
	dir_mknod,
	dir_symlink,
	dir_reparent
};





/*
 * oskit_comsid methods
 */

static OSKIT_COMDECL
sid_query(oskit_comsid_t * s,
	  const struct oskit_guid * iid,
	  void **out_ihandle)
{
	struct sfiledir *sfile;

	sfile = (struct sfiledir *) ((char *) s -
				     offsetof(struct sfiledir, sidi));

	return oskit_file_query(&sfile->filei, iid, out_ihandle);
}


static OSKIT_COMDECL_U
sid_addref(oskit_comsid_t * s)
{
	struct sfiledir *sfile;

	sfile = (struct sfiledir *) ((char *) s -
				     offsetof(struct sfiledir, sidi));

	return oskit_file_addref(&sfile->filei);
}


static OSKIT_COMDECL_U
sid_release(oskit_comsid_t * s)
{
	struct sfiledir *sfile;

	sfile = (struct sfiledir *) ((char *) s -
				     offsetof(struct sfiledir, sidi));

	return oskit_file_release(&sfile->filei);
}


static OSKIT_COMDECL
sid_get(oskit_comsid_t * s,
	oskit_security_class_t * outclass,
	oskit_security_id_t * outsid)
{
	struct sfiledir *sfile;

	sfile = (struct sfiledir *) ((char *) s -
				     offsetof(struct sfiledir, sidi));
	*outclass = sfile->sclass;
	*outsid = sfile->sid;
	return 0;
}


static void
revalidate_sopenfile(struct sfiledir * sfile,
		     struct sopenfile * sofile)
{
	oskit_error_t   rc;

	if (sofile->flags & OSKIT_O_RDONLY) {
		rc = SFS_FILE_HAS_PERM(sofile->sid, sfile, READ);
		if (rc) {
			sofile->flags &= ~OSKIT_O_RDONLY;
		}
	}
	if (sofile->flags & OSKIT_O_WRONLY) {
		rc = SFS_FILE_HAS_PERM(sofile->sid, sfile, WRITE);
		if (rc) {
			if (sofile->flags & OSKIT_O_APPEND)
				rc = SFS_FILE_HAS_PERM(sofile->sid, sfile, APPEND);
		}
		if (rc) {
			sofile->flags &= ~OSKIT_O_WRONLY;
		}
	}
}


static OSKIT_COMDECL
sid_set(oskit_comsid_t * s,
	oskit_security_id_t newsid)
{
	struct sfiledir *sfile;
	struct sopenfile *sofile;
	oskit_error_t   rc;

	sfile = (struct sfiledir *) ((char *) s -
				     offsetof(struct sfiledir, sidi));

	rc = oskit_filepsid_set(sfile->sfs->psid, sfile->file, newsid);
	if (rc)
		return (rc);

	sfile->sid = newsid;

	sofile = sfile->sopenfile_list;

	if (sofile == NULL)
		return 0;

	do {
		revalidate_sopenfile(sfile, sofile);
		sofile = sofile->next;
	} while (sofile != sfile->sopenfile_list);

	return 0;
}


static struct oskit_comsid_ops sid_ops = {
	sid_query,
	sid_addref,
	sid_release,
	sid_get,
	sid_set
};


/*
 * oskit_dir_secure methods
 */

static OSKIT_COMDECL
sdir_query(oskit_dir_secure_t * d,
	   const struct oskit_guid * iid,
	   void **out_ihandle)
{
	struct sfiledir *sdir;

	sdir = (struct sfiledir *) ((char *) d -
				    offsetof(struct sfiledir, sdiri));

	return oskit_file_query(&sdir->filei, iid, out_ihandle);
}


static OSKIT_COMDECL_U
sdir_addref(oskit_dir_secure_t * d)
{
	struct sfiledir *sdir;

	sdir = (struct sfiledir *) ((char *) d -
				    offsetof(struct sfiledir, sdiri));

	return oskit_file_addref(&sdir->filei);
}


static OSKIT_COMDECL_U
sdir_release(oskit_dir_secure_t * d)
{
	struct sfiledir *sdir;

	sdir = (struct sfiledir *) ((char *) d -
				    offsetof(struct sfiledir, sdiri));

	return oskit_file_release(&sdir->filei);
}


static OSKIT_COMDECL
sdir_create(oskit_dir_secure_t * d,
	    const char *name,
	    oskit_bool_t excl,
	    oskit_mode_t mode,
	    oskit_security_id_t fsid,
	    oskit_file_t ** out_file)
{
	struct sfiledir *sdir, *sfile;
	oskit_security_id_t   csid;
	oskit_file_t   *file;
	oskit_error_t   rc;

	sdir = (struct sfiledir *) ((char *) d -
				    offsetof(struct sfiledir, sdiri));

	rc = dir_lookup((oskit_dir_t *) & sdir->filei, name, out_file);
	if (!rc)
		return 0;

	if (rc != OSKIT_ENOENT)
		return rc;

	CSID(&csid);

	rc = file_create_checks(csid, sdir, OSKIT_SECCLASS_FILE, fsid);

	if (rc)
		return rc;

	rc = oskit_dir_create((oskit_dir_t *) sdir->file, name, excl, mode, &file);
	if (rc)
		return rc;

	rc = oskit_filepsid_set(sdir->sfs->psid, file, fsid);
	if (rc) {
		oskit_dir_unlink((oskit_dir_t *) sdir->file, name);
		oskit_file_release(file);
		return rc;
	}
	rc = sfiledir_create(sdir->sfs, file, &sfile);
	oskit_file_release(file);
	if (rc)
		return rc;

	*out_file = &sfile->filei;
	return 0;
}


static OSKIT_COMDECL
sdir_mkdir(oskit_dir_secure_t * sd,
	   const char *name,
	   oskit_mode_t mode,
	   oskit_security_id_t fsid)
{
	oskit_dir_t    *d;

	d = (oskit_dir_t *) ((char *) sd -
			     offsetof(struct sfiledir, sdiri));

	DIR_MKCALL(mkdir, d, OSKIT_SECCLASS_DIR, fsid, name, mode);
}


static struct oskit_dir_secure_ops sdir_ops = {
	sdir_query,
	sdir_addref,
	sdir_release,
	sdir_create,
	sdir_mkdir
};


/*
 * oskit_file_secure methods
 */

static OSKIT_COMDECL
sfile_query(oskit_file_secure_t * f,
	    const struct oskit_guid * iid,
	    void **out_ihandle)
{
	struct sfiledir *sfile;

	sfile = (struct sfiledir *) ((char *) f -
				     offsetof(struct sfiledir, sfilei));

	return oskit_file_query(&sfile->filei, iid, out_ihandle);
}


static OSKIT_COMDECL_U
sfile_addref(oskit_file_secure_t * f)
{
	struct sfiledir *sfile;

	sfile = (struct sfiledir *) ((char *) f -
				     offsetof(struct sfiledir, sfilei));
	return oskit_file_addref(&sfile->filei);
}


static OSKIT_COMDECL_U
sfile_release(oskit_file_secure_t * f)
{
	struct sfiledir *sfile;

	sfile = (struct sfiledir *) ((char *) f -
				     offsetof(struct sfiledir, sfilei));
	return oskit_file_release(&sfile->filei);
}

static inline oskit_error_t
file_open_checks(oskit_security_id_t csid,
		 struct sfiledir * sfile,
		 oskit_security_id_t ofsid,
		 oskit_oflags_t oflags)
{
	oskit_error_t   rc;


	if (csid != ofsid) {
		rc = oskit_avc_has_perm(sfile->sfs->avc, csid, ofsid,
					OSKIT_SECCLASS_FD,
					OSKIT_PERM_FD__CREATE);
		if (rc)
			return rc;
	}

	if (oflags & OSKIT_O_RDONLY) {
		rc = SFS_FILE_HAS_PERM(ofsid, sfile, READ);
		if (rc)
			return rc;
	}
	if (oflags & OSKIT_O_WRONLY) {
		if (sfile->filei.ops != &file_ops)
			return OSKIT_EISDIR;

		rc = SFS_FILE_HAS_PERM(ofsid, sfile, WRITE);
		if (rc) {
			if (oflags & OSKIT_O_APPEND)
				rc = SFS_FILE_HAS_PERM(ofsid, sfile, APPEND);
		}
		if (rc)
			return rc;
	}
	return 0;
}


static OSKIT_COMDECL
sfile_open(oskit_file_secure_t * f,
	   oskit_oflags_t flags,
	   oskit_security_id_t ofsid,
	   struct oskit_openfile ** out_openfile)
{
	struct sfiledir *sfile;
	oskit_openfile_t *ofile;
	struct sopenfile *sofile;
	oskit_security_id_t   csid;
	oskit_error_t   rc;

	sfile = (struct sfiledir *) ((char *) f -
				     offsetof(struct sfiledir, sfilei));
	CSID(&csid);

	rc = file_open_checks(csid, sfile, ofsid, flags);
	if (rc)
		return rc;

	rc = oskit_file_open(sfile->file, flags, &ofile);
	if (rc)
		return rc;
	if (!ofile) {
		rc = oskit_soa_openfile_from_absio(sfile->file, NULL, flags, &ofile);
		if (rc) {
			oskit_stream_t *stream;
			rc = oskit_file_query(sfile->file,
				      &oskit_stream_iid, (void **) &stream);
			if (rc) {
				rc = oskit_soa_openfile_from_stream(sfile->file, NULL, flags,
								    &ofile);
			} else {
				rc = oskit_soa_openfile_from_stream(sfile->file, stream, flags,
								    &ofile);
				oskit_stream_release(stream);
			}
		}
		if (rc)
			return rc;
	}
	rc = sopenfile_create(sfile, ofile, flags, ofsid, &sofile);
	oskit_openfile_release(ofile);
	if (rc)
		return rc;

	*out_openfile = &sofile->ofilei;
	return 0;
}


static OSKIT_COMDECL
sfile_chsid(oskit_file_secure_t * f,
	    oskit_security_id_t newsid)
{
	struct sfiledir *sfile;
	oskit_security_id_t   csid;
	oskit_security_id_t   fsid;
	oskit_error_t   rc;

	sfile = (struct sfiledir *) ((char *) f -
				     offsetof(struct sfiledir, sfilei));
	fsid = sfile->sid;

	CSID(&csid);

	rc = SFS_FILE_HAS_PERM(csid, sfile, RELABELFROM);
	if (rc)
		return rc;

	rc = oskit_avc_has_perm(sfile->sfs->avc, sfile->sid, newsid,
				sfile->sclass, OSKIT_PERM_FILE__TRANSITION);
	if (rc)
		return rc;

	rc = oskit_avc_has_perm(sfile->sfs->avc, csid, newsid,
				sfile->sclass, OSKIT_PERM_FILE__RELABELTO);
	if (rc)
		return rc;

	rc = SFS_FS_HAS_PERM(newsid, sfile->sfs, ASSOCIATE);
	if (rc)
		return rc;

	return oskit_comsid_set(&(sfile->sidi), newsid);
}

static struct oskit_file_secure_ops sfile_ops = {
	sfile_query,
	sfile_addref,
	sfile_release,
	sfile_open,
	sfile_chsid
};
