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
#include "sfs.h"

static struct oskit_openfile_ops ofile_ops;
static struct oskit_absio_ops ofile_absio_ops;
static struct oskit_comsid_ops sid_ops;

static inline struct sopenfile *
add_openfile(struct sopenfile * list,
	     struct sopenfile * item)
{
	if (list == NULL) {
		item->prev = item->next = item;
	} else {
		item->prev = list->prev;
		item->next = list;
		list->prev->next = item;
		list->prev = item;
	}
	return (item);
}

static inline struct sopenfile *
remove_openfile(struct sopenfile * list,
		struct sopenfile * item)
{
	if (item->next == item->prev) {
		return (NULL);
	} else {
		item->prev->next = item->next;
		item->next->prev = item->prev;
		return (item->next);
	}
}


oskit_error_t
sopenfile_create(struct sfiledir * sfile,
		 oskit_openfile_t * ofile,
		 oskit_oflags_t flags,
		 oskit_security_id_t sid,
		 struct sopenfile ** out_sofile)
{
	struct sopenfile *sofile;
	oskit_error_t   rc;


	sofile =
	  oskit_osenv_mem_alloc(sfile->sfs->mem,sizeof(struct sopenfile),0,0);
	if (!sofile)
		return OSKIT_ENOMEM;

	sofile->ofilei.ops = &ofile_ops;
	sofile->absioi.ops = &ofile_absio_ops;
	sofile->sidi.ops = &sid_ops;
	sofile->count = 1;
	sofile->flags = flags;
	sofile->sid = sid;
	sofile->sfile = sfile;
	oskit_file_addref(&sfile->filei);

	sofile->ofile = ofile;
	oskit_openfile_addref(ofile);

	rc = oskit_openfile_query(ofile, &oskit_absio_iid, (void **) &sofile->absio);
	if (rc)
		sofile->absio = NULL;

	sfile->sopenfile_list = add_openfile(sfile->sopenfile_list, sofile);

	OSKIT_AVC_ENTRY_REF_INIT(&sofile->avcr);
	OSKIT_AVC_ENTRY_REF_CPY(&sofile->file_avcr, &sfile->avcr);

	*out_sofile = sofile;

	return 0;
}


/*
 * oskit_openfile methods
 */

static OSKIT_COMDECL
ofile_query(oskit_openfile_t * f,
	    const struct oskit_guid * iid,
	    void **out_ihandle)
{
	struct sopenfile *sofile;

	sofile = (struct sopenfile *) f;

	if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
	    memcmp(iid, &oskit_stream_iid, sizeof(*iid)) == 0 ||
	    memcmp(iid, &oskit_openfile_iid, sizeof(*iid)) == 0) {
		*out_ihandle = &sofile->ofilei;
		++sofile->count;
		return 0;
	}
	if (memcmp(iid, &oskit_comsid_iid, sizeof(*iid)) == 0) {
		*out_ihandle = &sofile->sidi;
		++sofile->count;
		return 0;
	}
	if (sofile->absio && memcmp(iid, &oskit_absio_iid, sizeof(*iid)) == 0) {
		*out_ihandle = &sofile->absioi;
		++sofile->count;
		return 0;
	}
	*out_ihandle = NULL;
	return OSKIT_E_NOINTERFACE;
}


static OSKIT_COMDECL_U
ofile_addref(oskit_openfile_t * f)
{
	struct sopenfile *sofile;

	sofile = (struct sopenfile *) f;

	return ++sofile->count;
}


static OSKIT_COMDECL_U
ofile_release(oskit_openfile_t * f)
{
	struct sopenfile *sofile;
	struct sopenfile *openfile_list;
	unsigned        newcount;

	sofile = (struct sopenfile *) f;

	newcount = --sofile->count;
	if (newcount == 0) {
		openfile_list = sofile->sfile->sopenfile_list;
		sofile->sfile->sopenfile_list = remove_openfile(openfile_list, sofile);

		oskit_file_release(&sofile->sfile->filei);
		oskit_openfile_release(sofile->ofile);
		if (sofile->absio)
			oskit_absio_release(sofile->absio);

		oskit_osenv_mem_free(sofile->sfile->sfs->mem, sofile, 0,
				     sizeof(struct sopenfile));
	}
	return newcount;
}


static inline oskit_error_t
ofile_read_checks(struct sopenfile * sofile)
{
	oskit_security_id_t   csid;
	oskit_error_t   rc;

	if (!(sofile->flags & OSKIT_O_RDONLY))
		return OSKIT_EBADF;

	CSID(&csid);
	if (csid != sofile->sid) {
		rc = SFS_FD_HAS_PERM(csid, sofile, SETATTR);
		if (rc)
			return rc;

		rc = SFS_FD_FILE_HAS_PERM(csid, sofile, READ);
		if (rc)
			return rc;
	}
	return 0;
}


static OSKIT_COMDECL
ofile_read(oskit_openfile_t * f, void *buf, oskit_u32_t len,
	   oskit_u32_t * out_actual)
{
	struct sopenfile *sofile;
	oskit_error_t   rc;


	sofile = (struct sopenfile *) f;
	if (!sofile || !sofile->count)
		return OSKIT_E_INVALIDARG;

	rc = ofile_read_checks(sofile);
	if (rc)
		return rc;

	return oskit_openfile_read(sofile->ofile, buf, len, out_actual);
}


static inline oskit_error_t
ofile_write_checks(struct sopenfile * sofile)
{
	oskit_security_id_t   csid;
	oskit_error_t   rc;

	if (!(sofile->flags & OSKIT_O_WRONLY))
		return OSKIT_EBADF;

	CSID(&csid);
	if (csid != sofile->sid) {
		rc = SFS_FD_HAS_PERM(csid, sofile, SETATTR);
		if (rc)
			return rc;

		rc = SFS_FD_FILE_HAS_PERM(csid, sofile, WRITE);
		if (rc) {
			if (sofile->flags & OSKIT_O_APPEND) {
				rc = SFS_FD_FILE_HAS_PERM(csid, sofile, APPEND);
			}
		}

		if (rc)
			return rc;
	}
	return 0;
}


static OSKIT_COMDECL
ofile_write(oskit_openfile_t * f, const void *buf,
	    oskit_u32_t len, oskit_u32_t * out_actual)
{
	struct sopenfile *sofile;
	oskit_error_t   rc;

	sofile = (struct sopenfile *) f;
	if (!sofile || !sofile->count)
		return OSKIT_E_INVALIDARG;

	rc = ofile_write_checks(sofile);
	if (rc)
		return rc;

	return oskit_openfile_write(sofile->ofile, buf, len, out_actual);
}


static OSKIT_COMDECL
ofile_seek(oskit_openfile_t * f, oskit_s64_t ofs,
	   oskit_seek_t whence, oskit_u64_t * out_newpos)
{
	oskit_security_id_t   csid;
	struct sopenfile *sofile;
	oskit_error_t   rc;


	sofile = (struct sopenfile *) f;


	if (!sofile || !sofile->count)
		return OSKIT_E_INVALIDARG;

	CSID(&csid);
	if (csid != sofile->sid) {
		rc = SFS_FD_HAS_PERM(csid, sofile, SETATTR);
		if (rc)
			return rc;
	}
	return oskit_openfile_seek(sofile->ofile, ofs, whence, out_newpos);
}


static OSKIT_COMDECL
ofile_setsize(oskit_openfile_t * f, oskit_u64_t new_size)
{
	struct sopenfile *sofile;
	oskit_security_id_t   csid;
	oskit_error_t   rc;

	sofile = (struct sopenfile *) f;
	if (!sofile || !sofile->count)
		return OSKIT_E_INVALIDARG;

	CSID(&csid);
	rc = SFS_FD_FILE_HAS_PERM(csid, sofile, SETATTR);
	if (rc)
		return rc;

	return oskit_openfile_setsize(sofile->ofile, new_size);
}


static OSKIT_COMDECL
ofile_copy_to(oskit_openfile_t * f,
	      oskit_stream_t * dst,
	      oskit_u64_t size,
	      oskit_u64_t * out_read,
	      oskit_u64_t * out_written)
{
	return OSKIT_E_NOTIMPL;
}


static OSKIT_COMDECL
ofile_commit(oskit_openfile_t * f, oskit_u32_t commit_flags)
{
	return OSKIT_E_NOTIMPL;
}


static OSKIT_COMDECL
ofile_revert(oskit_openfile_t * f)
{
	return OSKIT_E_NOTIMPL;
}


static OSKIT_COMDECL
ofile_lock_region(oskit_openfile_t * f,
		  oskit_u64_t offset, oskit_u64_t size,
		  oskit_u32_t lock_type)
{
	return OSKIT_E_NOTIMPL;
}


static OSKIT_COMDECL
ofile_unlock_region(oskit_openfile_t * f,
		    oskit_u64_t offset, oskit_u64_t size,
		    oskit_u32_t lock_type)
{
	return OSKIT_E_NOTIMPL;
}


static OSKIT_COMDECL
ofile_stat(oskit_openfile_t * f, oskit_stream_stat_t * out_stat,
	   oskit_u32_t stat_flags)
{
	return OSKIT_E_NOTIMPL;
}


static OSKIT_COMDECL
ofile_clone(oskit_openfile_t * f, oskit_openfile_t ** out_stream)
{
	return OSKIT_E_NOTIMPL;
}


static OSKIT_COMDECL
ofile_getfile(oskit_openfile_t * f,
	      struct oskit_file ** out_file)
{
	struct sopenfile *sofile;


	sofile = (struct sopenfile *) f;
	if (!sofile || !sofile->count)
		return OSKIT_E_INVALIDARG;

	oskit_file_addref(&sofile->sfile->filei);
	*out_file = &sofile->sfile->filei;

	return 0;
}


static struct oskit_openfile_ops ofile_ops = {
	ofile_query,
	ofile_addref,
	ofile_release,
	ofile_read,
	ofile_write,
	ofile_seek,
	ofile_setsize,
	ofile_copy_to,
	ofile_commit,
	ofile_revert,
	ofile_lock_region,
	ofile_unlock_region,
	ofile_stat,
	ofile_clone,
	ofile_getfile
};




/*
 * oskit_openfile absolute I/O methods
 */

static OSKIT_COMDECL
afile_query(oskit_absio_t * io,
	    const struct oskit_guid * iid,
	    void **out_ihandle)
{
	struct sopenfile *ofile;

	if (!io || io->ops != &ofile_absio_ops)
		return OSKIT_E_INVALIDARG;

	ofile = (struct sopenfile *) ((char *) io -
				      offsetof(struct sopenfile, absioi));
	return oskit_openfile_query(&ofile->ofilei, iid, out_ihandle);
}


static OSKIT_COMDECL_U
afile_addref(oskit_absio_t * io)
{
	struct sopenfile *ofile;


	if (!io || io->ops != &ofile_absio_ops)
		return OSKIT_E_INVALIDARG;

	ofile = (struct sopenfile *) ((char *) io -
				      offsetof(struct sopenfile, absioi));
	return oskit_openfile_addref(&ofile->ofilei);
}


static OSKIT_COMDECL_U
afile_release(oskit_absio_t * io)
{
	struct sopenfile *ofile;

	if (!io || io->ops != &ofile_absio_ops)
		return OSKIT_E_INVALIDARG;

	ofile = (struct sopenfile *) ((char *) io -
				      offsetof(struct sopenfile, absioi));
	return oskit_openfile_release(&ofile->ofilei);
}


static OSKIT_COMDECL
afile_read(oskit_absio_t * io, void *buf,
	   oskit_off_t offset, oskit_size_t amount,
	   oskit_size_t * out_actual)
{
	struct sopenfile *sofile;
	oskit_error_t   rc;

	if (!io || io->ops != &ofile_absio_ops)
		return OSKIT_E_INVALIDARG;

	sofile = (struct sopenfile *) ((char *) io -
				       offsetof(struct sopenfile, absioi));
	if (!sofile->count)
		return OSKIT_E_INVALIDARG;

	rc = ofile_read_checks(sofile);
	if (rc)
		return rc;

	return oskit_absio_read(sofile->absio, buf, offset, amount, out_actual);
}


static OSKIT_COMDECL
afile_write(oskit_absio_t * io, const void *buf,
	    oskit_off_t offset, oskit_size_t amount,
	    oskit_size_t * out_actual)
{
	struct sopenfile *sofile;
	oskit_error_t   rc;

	if (!io || io->ops != &ofile_absio_ops)
		return OSKIT_E_INVALIDARG;

	sofile = (struct sopenfile *) ((char *) io -
				       offsetof(struct sopenfile, absioi));
	if (!sofile->count)
		return OSKIT_E_INVALIDARG;

	rc = ofile_write_checks(sofile);
	if (rc)
		return rc;

	return oskit_absio_write(sofile->absio, buf, offset, amount, out_actual);
}


static OSKIT_COMDECL
afile_get_size(oskit_absio_t * io, oskit_off_t * out_size)
{
	struct sopenfile *sofile;
	oskit_security_id_t   csid;
	oskit_error_t   rc;

	if (!io || io->ops != &ofile_absio_ops)
		return OSKIT_E_INVALIDARG;

	sofile = (struct sopenfile *) ((char *) io -
				       offsetof(struct sopenfile, absioi));
	if (!sofile->count)
		return OSKIT_E_INVALIDARG;

	CSID(&csid);
	rc = SFS_FD_FILE_HAS_PERM(csid, sofile, GETATTR);
	if (rc)
		return rc;

	return oskit_absio_getsize(sofile->absio, out_size);
}


static OSKIT_COMDECL
afile_set_size(oskit_absio_t * io, oskit_off_t new_size)
{
	struct sopenfile *sofile;
	oskit_security_id_t   csid;
	oskit_error_t   rc;

	if (!io || io->ops != &ofile_absio_ops)
		return OSKIT_E_INVALIDARG;

	sofile = (struct sopenfile *) ((char *) io -
				       offsetof(struct sopenfile, absioi));
	if (!sofile->count)
		return OSKIT_E_INVALIDARG;

	CSID(&csid);
	rc = SFS_FD_FILE_HAS_PERM(csid, sofile, SETATTR);
	if (rc)
		return rc;

	return oskit_absio_setsize(sofile->absio, new_size);
}


static struct oskit_absio_ops ofile_absio_ops = {
	afile_query,
	afile_addref,
	afile_release,
	(void *) 0,
	afile_read,
	afile_write,
	afile_get_size,
	afile_set_size
};




/*
 * oskit_comsid methods
 */

static OSKIT_COMDECL
sid_query(oskit_comsid_t * s,
	  const struct oskit_guid * iid,
	  void **out_ihandle)
{
	struct sopenfile *sofile;

	sofile = (struct sopenfile *) ((char *) s -
				       offsetof(struct sopenfile, sidi));

	return oskit_openfile_query(&sofile->ofilei, iid, out_ihandle);
}


static OSKIT_COMDECL_U
sid_addref(oskit_comsid_t * s)
{
	struct sopenfile *sofile;

	sofile = (struct sopenfile *) ((char *) s -
				       offsetof(struct sopenfile, sidi));
	return oskit_openfile_addref(&sofile->ofilei);
}


static OSKIT_COMDECL_U
sid_release(oskit_comsid_t * s)
{
	struct sopenfile *sofile;

	sofile = (struct sopenfile *) ((char *) s -
				       offsetof(struct sopenfile, sidi));
	return oskit_openfile_release(&sofile->ofilei);
}


static OSKIT_COMDECL
sid_get(oskit_comsid_t * s,
	oskit_security_class_t * outclass,
	oskit_security_id_t * outsid)
{
	struct sopenfile *sofile;

	sofile = (struct sopenfile *) ((char *) s -
				       offsetof(struct sopenfile, sidi));
	*outclass = OSKIT_SECCLASS_FD;
	*outsid = sofile->sid;
	return 0;
}


static OSKIT_COMDECL
sid_set(oskit_comsid_t * s,
	oskit_security_id_t newsid)
{
	struct sopenfile *sofile;

	sofile = (struct sopenfile *) ((char *) s -
				       offsetof(struct sopenfile, sidi));
	return OSKIT_E_NOTIMPL;
}


static struct oskit_comsid_ops sid_ops = {
	sid_query,
	sid_addref,
	sid_release,
	sid_get,
	sid_set
};
