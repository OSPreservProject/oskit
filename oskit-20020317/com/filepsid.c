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
#include <oskit/boolean.h>
#include <oskit/io/absio.h>
#include <oskit/fs/file.h>
#include <oskit/fs/dir.h>
#include <oskit/fs/openfile.h>
#include <oskit/fs/filepsid.h>
#include <oskit/com/sfs.h>
#include <oskit/com/services.h>
#include <oskit/dev/osenv.h>
#include <oskit/dev/osenv_mem.h>
#include <oskit/dev/osenv_log.h>
#include <oskit/machine/endian.h>
#include "sfs_new_hashtab.h"

#if BYTE_ORDER == LITTLE_ENDIAN
#define cpu_to_le32(x) (x)
#define le32_to_cpu(x) (x)
#define cpu_to_le64(x) (x)
#define le64_to_cpu(x) (x)
#endif

#define printf(fmt...) 	oskit_osenv_log_log(t->log, OSENV_LOG_INFO, ##fmt);

#if 1
#define DPRINTF(args...) printf("*** " args)
#else
#define DPRINTF(args...)
#endif

/*
 * Persistent security binding directory
 */
#define PSEC_SECDIR "...security"
#define PSEC_SECDIR_MODE 0700

/*
 * Persistent security binding files
 */
#define PSEC_CONTEXTS   0	/* security contexts */
#define PSEC_INDEX      1	/* psid -> (offset, len) of context */
#define PSEC_INODES	2	/* ino -> psid */
#define PSEC_NFILES	3	/* total number of security files */

static char    *psec_sfiles[PSEC_NFILES] =
{
	"contexts",
	"index",
	"inodes"
};

#define PSEC_SECFILE_MODE (OSKIT_S_IFREG | 0600)

/*
 * Record structure for entries in index file.
 */
typedef struct {
	oskit_off_t     ofs;	/* offset within file, in bytes */
	oskit_u32_t     len;	/* length of context, in bytes */
} s_index_t;

#define SINDEX_MASK (sizeof(s_index_t)-1)

typedef oskit_u32_t psid_t;

typedef struct psidtab_node {
	psid_t psid;
	oskit_security_id_t sid;
	struct psidtab_node *next;
} psidtab_node_t;

#define PSIDTAB_SLOTS 32

struct psidtab {
	oskit_filepsid_t psidi;
	unsigned        count;
	oskit_osenv_mem_t *mem;
	oskit_osenv_log_t *log;
	oskit_security_t  *security;
	psidtab_node_t *slots[PSIDTAB_SLOTS];
        hashtab_t ino2sid;
	psid_t next_psid;
	oskit_file_t  *files[PSEC_NFILES];
#define contexts_file absio[PSEC_CONTEXTS]
#define index_file absio[PSEC_INDEX]
#define inodes_file absio[PSEC_INODES]
	oskit_absio_t *absio[PSEC_NFILES];
#define contexts_absio absio[PSEC_CONTEXTS]
#define index_absio absio[PSEC_INDEX]
#define inodes_absio absio[PSEC_INODES]
};
typedef struct psidtab psidtab_t;

#define PSIDTAB_HASH(psid) (psid & (PSIDTAB_SLOTS - 1))

#define FILE_PSID 0
#define FS_PSID   1

static oskit_error_t psidtab_create(oskit_services_t *osenv,
				    oskit_security_t *security,
				    psidtab_t **tp)
{
	oskit_osenv_mem_t *mem;
	oskit_osenv_log_t *log;
	psidtab_t 	*t;

	oskit_services_lookup_first(osenv, &oskit_osenv_log_iid,
				    (void **) &log);
	if (!log)
		return OSKIT_EINVAL;

	oskit_services_lookup_first(osenv, &oskit_osenv_mem_iid,
				    (void **) &mem);
	if (!mem) {
		oskit_osenv_log_release(log);
		return OSKIT_EINVAL;
	}

	t = oskit_osenv_mem_alloc(mem, sizeof(psidtab_t), 0, 0);
	if (!t)
		return OSKIT_ENOMEM;
	memset(t, 0, sizeof(psidtab_t));

	t->mem = mem;
	oskit_osenv_mem_addref(mem);
	t->log = log;
	oskit_osenv_log_addref(log);

	t->security = security;
	oskit_security_addref(security);

	t->ino2sid = hashtab_create(mem);
	if (!t->ino2sid)
		return OSKIT_ENOMEM;

	*tp = t;
	return 0;
}


static void psidtab_destroy(psidtab_t *t)
{
	psidtab_node_t     *cur, *tmp;
	int             hvalue, i;
	oskit_osenv_mem_t *mem;


	for (i = 0; i < PSEC_NFILES; i++) {
		if (t->absio[i])
			oskit_absio_release(t->absio[i]);
		if (t->files[i]) {
			oskit_file_sync(t->files[i], TRUE);
			oskit_file_release(t->files[i]);
		}
	}

	for (hvalue = 0; hvalue < PSIDTAB_SLOTS; hvalue++) {
		cur = t->slots[hvalue];
		while (cur) {
			tmp = cur;
			cur = cur->next;
			oskit_osenv_mem_free(t->mem, tmp, 0,
					     sizeof(psidtab_node_t));
		}
	}

	hashtab_destroy(t->ino2sid);
	oskit_security_release(t->security);
	oskit_osenv_log_release(t->log);
	mem = t->mem;
	oskit_osenv_mem_free(mem, t, 0, sizeof(psidtab_t));
	oskit_osenv_mem_release(mem);
}


static int psidtab_insert(psidtab_t *t, psid_t psid, oskit_security_id_t sid)
{
	psidtab_node_t     *new;
	int             hvalue;


	hvalue = PSIDTAB_HASH(psid);
	new = oskit_osenv_mem_alloc(t->mem, sizeof(psidtab_node_t), 0, 0);
	if (!new) {
		return OSKIT_ENOMEM;
	}
	new->psid = psid;
	new->sid = sid;
	new->next = t->slots[hvalue];
	t->slots[hvalue] = new;
	return 0;
}


static oskit_security_id_t psidtab_search_psid(psidtab_t *t, psid_t psid)
{
	psidtab_node_t     *cur;
	int             hvalue;


	hvalue = PSIDTAB_HASH(psid);
	for (cur = t->slots[hvalue]; cur ; cur = cur->next) {
		if (psid == cur->psid)
			return cur->sid;
	}

	return 0;
}


static psid_t psidtab_search_sid(psidtab_t *t, oskit_security_id_t sid)
{
	psidtab_node_t     *cur;
	int             hvalue;


	for (hvalue = 0; hvalue < PSIDTAB_SLOTS; hvalue++) {
		for (cur = t->slots[hvalue]; cur; cur = cur->next) {
			if (sid == cur->sid && cur->psid > FS_PSID)
				return cur->psid;
		}
	}

	return 0;
}


static void psidtab_change_psid(psidtab_t *t,
				psid_t psid, oskit_security_id_t sid)
{
	psidtab_node_t     *cur;
	int             hvalue;


	hvalue = PSIDTAB_HASH(psid);
	for (cur = t->slots[hvalue]; cur ; cur = cur->next) {
		if (psid == cur->psid) {
			cur->sid = sid;
			return;
		}
	}

	return;
}


static oskit_error_t newpsid(psidtab_t *t,
			     oskit_security_id_t newsid,
			     psid_t *out_psid)
{
	oskit_security_context_t context;
	s_index_t raw_sindex;
	psid_t psid;
	oskit_u32_t actual, len;
	oskit_off_t off, off2;
	oskit_error_t rc;


	DPRINTF("newpsid:  obtaining psid for sid %ld\n", newsid);

	rc = oskit_security_sid_to_context(t->security,
					   newsid, &context, &len);
	if (rc)
		return OSKIT_EACCES;

	DPRINTF("newpsid:  sid %ld -> context %s\n", newsid, context);

	rc = oskit_absio_getsize(t->contexts_absio, &off);
	if (rc) {
	        oskit_osenv_mem_free(t->mem, context, OSENV_AUTO_SIZE, 0);
		return rc;
	}

	rc = oskit_absio_write(t->contexts_absio, context, off, len, &actual);
	if (rc) {
	        oskit_osenv_mem_free(t->mem, context, OSENV_AUTO_SIZE, 0);
		return rc;
	}
	if (actual != len) {
	        oskit_osenv_mem_free(t->mem, context, OSENV_AUTO_SIZE, 0);
		return OSKIT_EIO;
	}

	DPRINTF("newpsid:  added %s to contexts at %d\n",
		context, off);

        oskit_osenv_mem_free(t->mem, context, OSENV_AUTO_SIZE, 0);

	raw_sindex.ofs = cpu_to_le64(off);
	raw_sindex.len = cpu_to_le32(len);
	psid = t->next_psid++;

	off2 = psid * sizeof(s_index_t);
	rc = oskit_absio_write(t->index_absio, &raw_sindex, off2,
			       sizeof(s_index_t), &actual);
	if (rc)
		return rc;
	if (actual != sizeof(s_index_t))
		return OSKIT_EIO;

	DPRINTF("newpsid:  added (%d,%d) to index at %d for new psid %d\n",
		off, len, off2, psid);

	rc = psidtab_insert(t, psid, newsid);
	if (rc)
		return rc;

	DPRINTF("newpsid:  added (%d, %ld) to psidtab\n", psid, newsid);

	*out_psid = psid;
	return 0;
}


static struct oskit_filepsid_ops filepsid_ops;

oskit_error_t
oskit_filepsid_create(oskit_dir_t * rootdir,
		      oskit_security_id_t fs_sid,
		      oskit_security_id_t file_sid,
		      oskit_services_t *osenv,
		      oskit_security_t * security,
		      oskit_security_id_t *out_fs_sid,
		      oskit_filepsid_t ** out_filepsid)
{
	struct psidtab *t;
	oskit_dir_t    *dir = NULL;
	oskit_file_t   *file;
	oskit_openfile_t *ofile;
	oskit_u32_t 	index, actual, actual2;
	oskit_bool_t	need_to_init = FALSE;
	s_index_t	sindex, raw_sindex;
	oskit_security_id_t	sid;
	psid_t		file_psid, fs_psid;
	char		*cbuf;
	oskit_error_t	rc;
	oskit_off_t     off;


	rc = psidtab_create(osenv, security, &t);
	if (rc)
		return rc;

	t->psidi.ops = &filepsid_ops;
	t->count = 1;

	rc = oskit_dir_lookup(rootdir, PSEC_SECDIR, &file);
	if (rc) {
		if (rc != OSKIT_ENOENT) {
			psidtab_destroy(t);
			return rc;
		}
		DPRINTF("filepsid_create:  %s did not exist; creating\n", PSEC_SECDIR);

		rc = oskit_dir_mkdir(rootdir, PSEC_SECDIR, PSEC_SECDIR_MODE);
		if (rc) {
			psidtab_destroy(t);
			return rc;
		}
		rc = oskit_dir_lookup(rootdir, PSEC_SECDIR, &file);
		if (rc) {
			psidtab_destroy(t);
			return rc;
		}
		need_to_init = TRUE;
	}

	rc = oskit_file_query(file, &oskit_dir_iid, (void **) &dir);
	oskit_file_release(file);
	if (rc) {
		psidtab_destroy(t);
		return rc;
	}

	for (index = 0; index < PSEC_NFILES; index++) {
		DPRINTF("filepsid_create:  checking for %s\n",
			psec_sfiles[index]);

		rc = oskit_dir_lookup(dir, psec_sfiles[index],
				      &t->files[index]);
		if (rc) {
			if (rc != OSKIT_ENOENT) {
				goto out;
			}

			if (!need_to_init) {
				DPRINTF("filepsid_create:  %s did not exist\n", psec_sfiles[index]);
				goto out;
			}

			DPRINTF("filepsid_create:  %s did not exist; creating\n",
				psec_sfiles[index]);

			rc = oskit_dir_create(dir, psec_sfiles[index],
					      FALSE, PSEC_SECFILE_MODE,
					      &t->files[index]);
			if (rc)	{
				goto out;
			}
		}

		rc = oskit_file_open(t->files[index], OSKIT_O_RDWR, &ofile);
		if (rc) {
			goto out;
		}

		rc = oskit_openfile_query(ofile, &oskit_absio_iid,
					  (void **) &t->absio[index]);
		oskit_openfile_release(ofile);
		if (rc) {
			goto out;
		}
	}

	if (need_to_init) {
		DPRINTF("filepsid_create:  initializing labeling files\n");

		rc = newpsid(t, file_sid, &file_psid);
		if (rc)
			goto out;

		if (file_psid) {
			printf("filepsid_create:  default file psid should be zero, is %d\n", file_psid);
			goto out;
		}

		rc = newpsid(t, fs_sid, &fs_psid);
		if (rc)
			goto out;

		if (fs_psid != FS_PSID) {
			printf("filepsid_create:  file system psid should be %d, is %d\n", FS_PSID, fs_psid);
			goto out;
		}


		rc = oskit_filepsid_set(&t->psidi, (oskit_file_t *)dir,
					OSKIT_SECINITSID_FILE_LABELS);
		if (rc) {
			printf("filepsid_create:  unable to set labeling directory SID, rc 0x%x\n", rc);
			goto out;
		}

		rc = oskit_dir_sync(dir, TRUE);
		if (rc) {
			printf("filepsid_create:  unable to sync labeling directory, rc 0x%x\n", rc);
			goto out;
		}
		oskit_dir_release(dir);

		for (index = 0; index < PSEC_NFILES; index++) {
			rc = oskit_filepsid_set(&t->psidi, t->files[index],
					   OSKIT_SECINITSID_FILE_LABELS);
			if (rc) {
				printf("filepsid_create:  unable to set labeling file SID, rc 0x%x\n", rc);
				goto out;
			}

			rc = oskit_file_sync(t->files[index], TRUE);
			if (rc) {
				printf("filepsid_create:  unable to sync labeling file, rc 0x%x\n", rc);
				goto out;
			}
		}

		*out_fs_sid = fs_sid;
		*out_filepsid = &t->psidi;

		return 0;
	}

	DPRINTF("filepsid_create:  reading existing labeling files\n");

	off = 0;
	rc = oskit_absio_read(t->index_absio, &raw_sindex, off,
			      sizeof raw_sindex, &actual);
	while (!rc && (actual == sizeof raw_sindex)) {
		sindex.ofs = le64_to_cpu(raw_sindex.ofs);
		sindex.len = le32_to_cpu(raw_sindex.len);

		DPRINTF("filepsid_create:  read index (%d,%d) for psid %d\n",
			sindex.ofs, sindex.len, t->next_psid);

		cbuf = oskit_osenv_mem_alloc(t->mem, sindex.len, 0, 0);
		if (!cbuf) {
			rc = OSKIT_ENOMEM;
			goto out;
		}

		rc = oskit_absio_read(t->contexts_absio, cbuf,
				      sindex.ofs, sindex.len, &actual2);
		if (rc)
			goto out;
		if (actual2 != sindex.len) {
			rc = OSKIT_EIO;
			goto out;
		}

		rc = oskit_security_context_to_sid(t->security,
						   cbuf, sindex.len, &sid);
		if (rc) {
			DPRINTF("filepsid_create:  error 0x%x in obtaining SID for context %s (psid %d), skipping.\n",
				rc, cbuf, t->next_psid);

			t->next_psid++;
		} else {
			DPRINTF("filepsid_create:  psid %d -> context %s -> sid %d\n",
				t->next_psid, cbuf, sid);

			rc = psidtab_insert(t, t->next_psid, sid);
			t->next_psid++;
			if (rc)
				goto out;
		}

		oskit_osenv_mem_free(t->mem, cbuf, 0, sindex.len);

		off += sizeof raw_sindex;
		rc = oskit_absio_read(t->index_absio,
				      &raw_sindex, off,
				      sizeof raw_sindex, &actual);
	}

	if (rc < 0) {
		printf("filepsid_create:  error 0x%x in reading index\n",
		       rc);
		goto out;
	}

	if (actual && actual != sizeof raw_sindex) {
		printf("filepsid_create:  partial entry in index\n");
		rc = OSKIT_EIO;
		goto out;
	}

	fs_sid = psidtab_search_psid(t, FS_PSID);
	if (!fs_sid) {
		printf("filepsid_create:  no SID for fs psid\n");
		rc = OSKIT_EINVAL;
		goto out;
	}

	DPRINTF("filepsid_create:  fs sid %ld\n", fs_sid);

	*out_fs_sid = fs_sid;
	*out_filepsid = &t->psidi;

out:
	if (dir)
		oskit_dir_release(dir);
	if (rc)
		psidtab_destroy(t);
	return rc;
}


/*
 * oskit_filepsid methods
 */

static OSKIT_COMDECL
fpsid_query(oskit_filepsid_t * p,
	    const struct oskit_guid * iid,
	    void **out_ihandle)
{
	struct psidtab *t;

	t = (struct psidtab *) p;

	if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
	    memcmp(iid, &oskit_filepsid_iid, sizeof(*iid)) == 0) {
		*out_ihandle = &t->psidi;
		++t->count;
		return 0;
	}
	*out_ihandle = NULL;
	return OSKIT_E_NOINTERFACE;
}


static OSKIT_COMDECL_U
fpsid_addref(oskit_filepsid_t * p)
{
	struct psidtab *t;

	t = (struct psidtab *) p;

	return ++t->count;
}


static OSKIT_COMDECL_U
fpsid_release(oskit_filepsid_t * p)
{
	struct psidtab *t;
	unsigned        newcount;

	t = (struct psidtab *) p;

	newcount = --t->count;
	if (newcount == 0) {
		psidtab_destroy(t);
	}
	return newcount;
}


static OSKIT_COMDECL
fpsid_get(oskit_filepsid_t * p,
	  oskit_file_t * file,
	  oskit_security_id_t * outsid)
{
	struct psidtab *t;
	psid_t          raw_psid, psid = 0;
	oskit_security_id_t   sid;
	oskit_stat_t    stats;
	oskit_u32_t     actual;
	oskit_error_t   rc;


	t = (struct psidtab *) p;

	rc = oskit_file_stat(file, &stats);
	if (rc)
		return rc;

	sid = (oskit_security_id_t) hashtab_search(t->ino2sid,
					     (hashtab_key_t) stats.ino);
	if (!sid) {
		rc = oskit_absio_read(t->inodes_absio,
				      &raw_psid,
				      stats.ino * sizeof(psid_t),
				      sizeof(psid_t),
				      &actual);
		if (rc)
			return rc;
		if (actual != sizeof(psid_t))
			return OSKIT_EIO;

		psid = le32_to_cpu(raw_psid);

		sid = psidtab_search_psid(t, psid);
		if (!sid) {
			/*
			 * If the psid is undefined, then reset it to PSID 0.
			 */
			printf("psid_to_sid:  no SID for psid %d\n", psid);
			sid = psidtab_search_psid(t, 0);
			if (!sid) {
				DPRINTF("psid_to_sid:  psid %d -> unlabeled\n", psid);
				*outsid = OSKIT_SECINITSID_UNLABELED;
				return 0;
			}
			DPRINTF("psid_to_sid:  replacing psid %d with psid 0\n", psid);
			oskit_filepsid_set(p, file, sid);
		}

		rc = hashtab_insert(t->ino2sid,
				    (hashtab_key_t) stats.ino,
				    (hashtab_datum_t) sid);
		if (rc)
			return rc;
	}

	*outsid = sid;
	return 0;
}


static OSKIT_COMDECL
fpsid_set(oskit_filepsid_t * p,
	  oskit_file_t * file,
	  oskit_security_id_t newsid)
{
	struct psidtab *t;
	oskit_stat_t    stats;
	psid_t          psid, raw_psid;
	oskit_error_t   rc;
	oskit_u32_t     actual;


	t = (struct psidtab *) p;

	rc = oskit_file_stat(file, &stats);
	if (rc)
		return rc;

	psid = psidtab_search_sid(t, newsid);
	if (!psid) {
		rc = newpsid(t, newsid, &psid);
		if (rc)
			return rc;
	}

	raw_psid = cpu_to_le32(psid);

	rc = oskit_absio_write(t->inodes_absio,
			       &raw_psid,
			       stats.ino * sizeof(psid_t),
			       sizeof(psid_t),
			       &actual);
	if (rc)
		return rc;
	if (actual != sizeof(psid_t))
		return OSKIT_EIO;

	return hashtab_replace(t->ino2sid,
			       (hashtab_key_t) stats.ino,
			       (hashtab_datum_t) newsid);
}


static int chpsid(psidtab_t *t,
		  psid_t psid,
		  oskit_security_id_t newsid)
{
	oskit_security_context_t context;
	s_index_t raw_sindex;
	psid_t clone_psid;
	oskit_u32_t len, actual;
	oskit_off_t off, off2;
	int rc;
	psidtab_node_t     *cur;
	int             hvalue;


	DPRINTF("chpsid:  changing psid %d to sid %ld\n", psid, newsid);

	for (hvalue = 0; hvalue < PSIDTAB_SLOTS; hvalue++) {
		for (cur = t->slots[hvalue]; cur; cur = cur->next) {
			if (newsid == cur->sid)
				goto found;
		}
	}

found:
	if (hvalue < PSIDTAB_SLOTS) {
		clone_psid = cur->psid;
		DPRINTF("chpsid:  cloning existing psid %d\n", clone_psid);
		off2 = clone_psid * sizeof(s_index_t);
		rc = oskit_absio_read(t->index_absio,
				      &raw_sindex,
				      off2,
				      sizeof(s_index_t),
				      &actual);
		if (rc < 0) {
			printf("chpsid:  error 0x%x in reading index\n",
			       rc);
			return rc;
		}
		if (actual != sizeof(s_index_t)) {
			printf("chpsid:  partial entry in index\n");
			return OSKIT_EIO;
		}

		off = le64_to_cpu(raw_sindex.ofs);
		len = le32_to_cpu(raw_sindex.len);
	}
	else {
		DPRINTF("chpsid:  adding new entry to contexts\n");
		rc = oskit_security_sid_to_context(t->security,
						   newsid, &context, &len);
		if (rc)
			return OSKIT_EACCES;

		DPRINTF("chpsid:  sid %ld -> context %s\n", newsid, context);

		rc = oskit_absio_getsize(t->contexts_absio, &off);
		if (rc) {
		        oskit_osenv_mem_free(t->mem, context, OSENV_AUTO_SIZE, 0);
			return rc;
		}

		rc = oskit_absio_write(t->contexts_absio,
				       context, off, len, &actual);
		if (rc < 0) {
        		oskit_osenv_mem_free(t->mem, context, OSENV_AUTO_SIZE, 0);
			printf("chpsid:  error 0x%x in writing to contexts\n", rc);
			return rc;
		}
		if (actual != len) {
		        oskit_osenv_mem_free(t->mem, context, OSENV_AUTO_SIZE, 0);
			printf("chpsid:  partial entry in contexts\n");
			return OSKIT_EIO;
		}

		raw_sindex.ofs = cpu_to_le64(off);
		raw_sindex.len = cpu_to_le32(len);

		DPRINTF("chpsid:  added %s to contexts at %d\n",
			context, off);

	        oskit_osenv_mem_free(t->mem, context, OSENV_AUTO_SIZE, 0);
	}

	off2 = psid * sizeof(s_index_t);
	rc = oskit_absio_write(t->index_absio, &raw_sindex, off2,
			       sizeof(s_index_t), &actual);
	if (rc < 0) {
		printf("chpsid:  error 0x%x in writing to index\n", rc);
		return rc;
	}
	if (actual != sizeof(s_index_t)) {
		printf("chpsid:  partial write to index\n");
		return OSKIT_EIO;
	}

	DPRINTF("chpsid:  changed index at %d to (%d, %d) for psid %d\n",
		off2, off, len, psid);

	psidtab_change_psid(t, psid, newsid);

	DPRINTF("chpsid:  changed to (%d, %ld) in psidtab\n", psid, newsid);

	return 0;
}


static OSKIT_COMDECL
fpsid_setfs(oskit_filepsid_t * p,
	    oskit_security_id_t fs_sid,
	    oskit_security_id_t f_sid)
{
	struct psidtab *t;
	int rc = 0;

	t = (struct psidtab *) p;

	if (fs_sid) {
		rc = chpsid(t, FS_PSID, fs_sid);
	}
	if (f_sid && !rc) {
		rc = chpsid(t, FILE_PSID, f_sid);
	}
	return rc;
}


static struct oskit_filepsid_ops filepsid_ops = {
	fpsid_query,
	fpsid_addref,
	fpsid_release,
	fpsid_get,
	fpsid_set,
	fpsid_setfs
};
