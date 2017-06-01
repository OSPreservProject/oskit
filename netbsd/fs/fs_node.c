/*
 * Copyright (c) 1997-1999 University of Utah and the Flux Group.
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

/*
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "fs_glue.h"
#include <oskit/c/string.h>

#define offsetof(type, member) ((size_t)(&((type *)0)->member))


/*
 * oskit_file and oskit_dir methods; per-node operations
 */

/*
 * Internal representation of a single dirent entry.
 */
typedef struct internal_dirent {
	oskit_size_t	namelen;		/* length of name */
	oskit_ino_t	ino;			/* entry inode */
	char	       *name;			/* entry name */
} internal_dirent_t;
static oskit_error_t	fs_dirents_create(internal_dirent_t *entries,
				int count, oskit_dirents_t **out_dirents);

/*
 * vnode-based helper functions for both oskit_file and oskit_dir methods
 */

static oskit_error_t vp_stat(struct vnode *vp,
			    struct oskit_stat *out_stats)
{
    struct stat sb;
    struct proc *p;
    oskit_error_t ferror;
    int error;

    ferror = getproc(&p);
    if (ferror)
	    return ferror;

    VOP_LOCK(vp);
    error = vn_stat(vp, &sb, p);
    VOP_UNLOCK(vp);
    prfree(p);
    if (error)
	    return errno_to_oskit_error(error);

    out_stats->dev = sb.st_dev;
    out_stats->ino = sb.st_ino;
    out_stats->mode = sb.st_mode;
    out_stats->nlink = sb.st_nlink;
    out_stats->uid = sb.st_uid;
    out_stats->gid = sb.st_gid;
    out_stats->rdev = sb.st_rdev;
    out_stats->atime.tv_sec = sb.st_atime;
    out_stats->atime.tv_nsec = sb.st_atimensec;
    out_stats->mtime.tv_sec = sb.st_mtime;
    out_stats->mtime.tv_nsec = sb.st_mtimensec;
    out_stats->ctime.tv_sec = sb.st_ctime;
    out_stats->ctime.tv_nsec = sb.st_ctimensec;
    out_stats->size = sb.st_size;
    out_stats->blocks = sb.st_blocks;
    out_stats->blksize = sb.st_blksize;

    return 0;
}


static oskit_error_t vp_setstat(struct vnode *vp, oskit_u32_t mask,
			       const struct oskit_stat *stats)
{
    struct vattr vattr;
    struct proc *p;
    oskit_error_t ferror;
    int error;

    if (mask & ~OSKIT_STAT_SETTABLE)
	    return OSKIT_E_INVALIDARG;

    if (vp->v_type == VLNK)
	    return OSKIT_E_NOTIMPL;

    ferror = getproc(&p);
    if (ferror)
	    return ferror;

    error = 0;
    VOP_LEASE(vp, p, p->p_ucred, LEASE_WRITE);
    VOP_LOCK(vp);
    if (vp->v_mount->mnt_flag & MNT_RDONLY)
	    error = EROFS;
    else {
	VATTR_NULL(&vattr);

	if (mask & OSKIT_STAT_MODE)
		vattr.va_mode = stats->mode & ALLPERMS;
	if (mask & OSKIT_STAT_UID)
		vattr.va_uid = stats->uid;
	if (mask & OSKIT_STAT_GID)
		vattr.va_gid = stats->gid;

	if (mask & OSKIT_STAT_SIZE)
	{
	    vattr.va_size = stats->size;

	    if (vp->v_type == VDIR)
		    error = EISDIR;
	    else
	    {
		error = vn_writechk(vp);
		if (!error)
			error = VOP_ACCESS(vp, VWRITE, p->p_ucred, p);
	    }
	}

	if (mask & OSKIT_STAT_UTIMES_NULL)
	{
	    vattr.va_vaflags |= VA_UTIMES_NULL;
	    TIMEVAL_TO_TIMESPEC(&time, &vattr.va_atime);
	    vattr.va_mtime = vattr.va_atime;
	}
	else
	{
	    if (mask & OSKIT_STAT_ATIME)
	    {
		vattr.va_atime.tv_sec = stats->atime.tv_sec;
		vattr.va_atime.tv_nsec = stats->atime.tv_nsec;
	    }

	    if (mask & OSKIT_STAT_MTIME)
	    {
		vattr.va_mtime.tv_sec = stats->mtime.tv_sec;
		vattr.va_mtime.tv_nsec = stats->mtime.tv_nsec;
	    }
	}

	if (!error)
		error = VOP_SETATTR(vp, &vattr, p->p_ucred, p);
    }

    VOP_UNLOCK(vp);
    prfree(p);
    if (error)
	    return errno_to_oskit_error(error);

    return 0;
}


static oskit_error_t vp_pathconf(struct vnode *vp, oskit_s32_t option,
				oskit_s32_t *out_val)
{
    int error;


    if (vp->v_type == VLNK)
	    return OSKIT_E_NOTIMPL;

    /*
     * We can get by without a curproc here.
     * VOP_LOCK uses curproc only if non-NULL.
     */
    VOP_LOCK(vp);
    error = VOP_PATHCONF(vp, option, (register_t *) out_val);
    VOP_UNLOCK(vp);
    if (error)
	    return errno_to_oskit_error(error);

    return 0;
}


static oskit_error_t vp_sync(struct vnode *vp, oskit_bool_t wait)
{
    oskit_error_t ferror;
    struct proc *p;
    int error;


    if (vp->v_type == VLNK)
	    return OSKIT_E_NOTIMPL;

    ferror = getproc(&p);
    if (ferror)
	    return ferror;

    VOP_LOCK(vp);
    error = VOP_FSYNC(vp, p->p_ucred, wait ? MNT_WAIT : 0, p);
    VOP_UNLOCK(vp);
    prfree(p);
    if (error)
	    return errno_to_oskit_error(error);

    return 0;
}


static oskit_error_t vp_access(struct vnode *vp, oskit_amode_t mask)
{
    oskit_error_t ferror;
    struct proc *p;
    int error, flags;

    if (vp->v_type == VLNK)
	    return OSKIT_E_NOTIMPL;

    ferror = getproc(&p);
    if (ferror)
	    return ferror;

    error = 0;
    if (mask)
    {
	flags = 0;
	if (mask & OSKIT_R_OK)
		flags |= VREAD;
	if (mask & OSKIT_W_OK)
		flags |= VWRITE;
	if (mask & OSKIT_X_OK)
		flags |= VEXEC;
	VOP_LOCK(vp);
	if ((flags & VWRITE) == 0 || (error = vn_writechk(vp)) == 0)
		error = VOP_ACCESS(vp, flags, p->p_ucred, p);
	VOP_UNLOCK(vp);
    }

    prfree(p);
    if (error)
	    return errno_to_oskit_error(error);

    return 0;
}


static int myfalloc(struct ucred *cred, struct file **resultfp)
{
    register struct file *fp;

    if (nfiles >= maxfiles) {
	tablefull("file");
	return (ENFILE);
    }
    /*
     * Allocate a new file descriptor.
     * Add it to the list of open files.
     */
    nfiles++;
    MALLOC(fp, struct file *, sizeof(struct file), M_FILE, M_WAITOK);
    bzero(fp, sizeof(struct file));
    LIST_INSERT_HEAD(&filehead, fp, f_list);
    fp->f_count = 1;
    fp->f_cred = cred;
    crhold(fp->f_cred);
    if (resultfp)
	    *resultfp = fp;
    return (0);
}


static struct fileops vnops =
           { vn_read, vn_write, vn_ioctl, vn_select, vn_closefile };

static oskit_error_t vp_open(struct vnode *vp, oskit_oflags_t iflags,
			    struct file **out_fp)
{
    oskit_error_t ferror;
    struct proc *p;
    int error, flags;
    struct file *fp;
    struct vattr va;

    if (vp->v_type != VREG && vp->v_type != VDIR)
	    return OSKIT_E_NOTIMPL;

    ferror = getproc(&p);
    if (ferror)
	    return ferror;

    VOP_LOCK(vp);

    flags = 0;
    if (iflags & OSKIT_O_RDONLY)
	    flags |= FREAD;
    if (iflags & OSKIT_O_WRONLY)
	    flags |= FWRITE;
    if (iflags & OSKIT_O_TRUNC)
	    flags |= O_TRUNC;
    if (iflags & OSKIT_O_APPEND)
	    flags |= FAPPEND;

    if (flags & FREAD)
    {
	if ((error = VOP_ACCESS(vp, VREAD, p->p_ucred, p)) != 0)
		goto bad;
    }

    if (flags & (FWRITE | O_TRUNC)) {
	if (vp->v_type == VDIR) {
	    error = EISDIR;
	    goto bad;
	}

	if ((error = vn_writechk(vp)) != 0 ||
	    (error = VOP_ACCESS(vp, VWRITE, p->p_ucred, p)) != 0)
		goto bad;

	if (flags & O_TRUNC) {
	    VATTR_NULL(&va);
	    va.va_size = 0;
	    if ((error = VOP_SETATTR(vp, &va, p->p_ucred, p)) != 0)
		    goto bad;
	}
    }

    if ((error = VOP_OPEN(vp, flags, p->p_ucred, p)) != 0)
	    goto bad;
    if (flags & FWRITE)
	    vp->v_writecount++;

    error = myfalloc(p->p_ucred, &fp);
    prfree(p);
    if (error)
	    goto bad;

    fp->f_flag = flags & FMASK;
    fp->f_type = DTYPE_VNODE;
    fp->f_ops = &vnops;
    fp->f_data = (caddr_t)vp;
    VREF(vp);
    VOP_UNLOCK(vp);

    *out_fp = fp;

    return 0;

bad:
    prfree(p);
    VOP_UNLOCK(vp);
    return errno_to_oskit_error(error);
}



/*
 * oskit_file internal constructor
 */

static struct oskit_file_ops file_ops;
static struct oskit_dir_ops dir_ops;
static struct oskit_absio_ops file_absio_ops;

struct gfile *gfile_create(struct gfilesystem *fs, struct vnode *vp)
{
    struct gfile *file;
    struct gdir *dir;
    int error;

    if (vp->v_type == VDIR) {
	    /*
	     * This file is a directory, so create a directory object,
	     * which is a superset of a file object.
	     */
	    MALLOC(dir,struct gdir*,sizeof(struct gdir),M_TEMP,M_WAITOK);
	    if (!dir)
		    return NULL;
	    file = (struct gfile *) dir;
	    /*
	     * Initialize the directory-specific fields.
	     */
	    dir->diri.ops = &dir_ops;
	    dir->virtual = FALSE;
	    dir->parent = NULL;
    }
    else {
	    MALLOC(file,struct gfile*,sizeof(struct gfile),M_TEMP,M_WAITOK);
	    if (!file)
		    return NULL;
	    file->filei.ops = &file_ops;
    }

    file->f.absioi.ops = &file_absio_ops;
    file->f.count = 1;
    file->f.fs = fs;
    file->f.vp = vp;

    error = hashtab_insert(vptab, (hashtab_key_t) vp, (hashtab_datum_t) file);
    if (error)
    {
	FREE(file, M_TEMP);
	return NULL;
    }

    oskit_filesystem_addref(&fs->fsi);	
    VREF(vp);				

    return file;
}


/*
 * oskit_file methods
 */

static OSKIT_COMDECL file_query(oskit_file_t *f,
			       const struct oskit_guid *iid,
			       void **out_ihandle)
{
    struct gfile *file;

    file = (struct gfile*)f;
    if (file == NULL)
	    panic("%s:%d: null file", __FILE__, __LINE__);
    if (file->f.count == 0)
	    panic("%s:%d: bad count", __FILE__, __LINE__);

    if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
	memcmp(iid, &oskit_posixio_iid, sizeof(*iid)) == 0 ||
	memcmp(iid, &oskit_file_iid, sizeof(*iid)) == 0) {
	*out_ihandle = &file->filei;
	++file->f.count;
	return 0;
    }

    if (memcmp(iid, &oskit_absio_iid, sizeof(*iid)) == 0) {
	*out_ihandle = &file->f.absioi;
	++file->f.count;
	return 0;
    }

    *out_ihandle = NULL;
    return OSKIT_E_NOINTERFACE;
}


static OSKIT_COMDECL_U file_addref(oskit_file_t *f)
{
    struct gfile *file;

    file = (struct gfile*)f;
    if (file == NULL)
	    panic("%s:%d: null file", __FILE__, __LINE__);
    if (file->f.count == 0)
	    panic("%s:%d: bad count", __FILE__, __LINE__);

    return ++file->f.count;
}


static OSKIT_COMDECL_U file_release(oskit_file_t *f)
{
    struct gfile *file;
    int rc;
    unsigned newcount;

    file = (struct gfile*)f;
    if (file == NULL)
	    panic("%s:%d: null file", __FILE__, __LINE__);
    if (file->f.count == 0)
	    panic("%s:%d: bad count", __FILE__, __LINE__);

    newcount = --file->f.count;
    if (newcount == 0)
    {
	int ferror;
	struct proc *p;

	oskit_filesystem_release(&file->f.fs->fsi);

	rc = hashtab_remove(vptab, (hashtab_key_t)file->f.vp, 0, 0);
	assert(rc == HASHTAB_SUCCESS);

	ferror = getproc(&p);
	assert(!ferror);
	vrele(file->f.vp);
	prfree(p);
	free(file,M_TEMP);
    }

    return newcount;
}


static OSKIT_COMDECL file_stat(oskit_file_t *f,
			     struct oskit_stat *out_stats)
{
    struct gfile *file;

    file = (struct gfile *)f;

    if (!file || !file->f.count)
	    return OSKIT_E_INVALIDARG;

    return vp_stat(file->f.vp,out_stats);
}


static OSKIT_COMDECL file_setstat(oskit_file_t *f, oskit_u32_t mask,
				const struct oskit_stat *stats)
{
    struct gfile *file;

    file = (struct gfile *)f;

    if (!file || !file->f.count)
	    return OSKIT_E_INVALIDARG;

    return vp_setstat(file->f.vp,mask,stats);
}


static OSKIT_COMDECL file_pathconf(oskit_file_t *f, oskit_s32_t option,
				 oskit_s32_t *out_val)
{
    struct gfile *file;

    file = (struct gfile *)f;

    if (!file || !file->f.count)
	    return OSKIT_E_INVALIDARG;

    return vp_pathconf(file->f.vp,option,out_val);
}


static OSKIT_COMDECL file_sync(oskit_file_t *f, oskit_bool_t wait)
{
    struct gfile *file;

    file = (struct gfile *)f;

    if (!file || !file->f.count)
	    return OSKIT_E_INVALIDARG;

    return vp_sync(file->f.vp,wait);
}


static OSKIT_COMDECL file_access(oskit_file_t *f, oskit_amode_t mask)
{
    struct gfile *file;

    file = (struct gfile *)f;

    if (!file || !file->f.count)
	    return OSKIT_E_INVALIDARG;

    return vp_access(file->f.vp,mask);
}


static OSKIT_COMDECL file_readlink(oskit_file_t *f,
				  char *buf, oskit_u32_t len,
				  oskit_u32_t *out_actual)
{
    struct gfile *file;
    oskit_error_t ferror;
    struct proc *p;
    struct iovec aiov;
    struct uio auio;
    int error;

    file = (struct gfile *)f;
    if (!file || !file->f.count)
	    return OSKIT_E_INVALIDARG;

    if (file->f.vp->v_type != VLNK)
	    return OSKIT_E_NOTIMPL;

    ferror = getproc(&p);
    if (ferror)
	    return ferror;

    aiov.iov_base = buf;
    aiov.iov_len = len;
    auio.uio_iov = &aiov;
    auio.uio_iov = &aiov;
    auio.uio_iovcnt = 1;
    auio.uio_offset = 0;
    auio.uio_rw = UIO_READ;
    auio.uio_segflg = UIO_SYSSPACE;
    auio.uio_procp = p;
    auio.uio_resid = len;
    VOP_LOCK(file->f.vp);
    error = VOP_READLINK(file->f.vp, &auio, p->p_ucred);
    VOP_UNLOCK(file->f.vp);
    *out_actual = len - auio.uio_resid;
    prfree(p);
    return errno_to_oskit_error(error);
}


static OSKIT_COMDECL file_open(oskit_file_t *f, oskit_oflags_t iflags,
			     struct oskit_openfile **out_openfile)
{
    struct gfile *file;
    struct gopenfile *ofile;
    oskit_error_t ferror;
    struct file *fp;

    file = (struct gfile *)f;

    if (!file || !file->f.count)
	    return OSKIT_E_INVALIDARG;

    ferror = vp_open(file->f.vp, iflags, &fp);
    if (ferror)
	    return ferror;

    ofile = gopenfile_create(file, fp);
    if (!ofile)
    {
	closef(fp,NULL);
	return OSKIT_ENOMEM;
    }

    *out_openfile = &ofile->ofilei;
    return 0;
}


static OSKIT_COMDECL file_getfs(oskit_file_t *f, struct oskit_filesystem **out_fs)
{
    struct gfile *file;

    file = (struct gfile *)f;

    if (!file || !file->f.count)
	    return OSKIT_E_INVALIDARG;

    oskit_filesystem_addref(&file->f.fs->fsi);
    *out_fs = &file->f.fs->fsi;

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

static OSKIT_COMDECL afile_query(oskit_absio_t *io,
				const struct oskit_guid *iid,
				void **out_ihandle)
{
    struct gfile *file;

    if (!io || io->ops != &file_absio_ops)
	    return OSKIT_E_INVALIDARG;

    file = (struct gfile *) ((char *) io - offsetof(struct gfile, f));
    if (!file->f.count)
	    return OSKIT_E_INVALIDARG;

    /* may be file_query or dir_query */
    return oskit_file_query(&file->filei, iid, out_ihandle);
}

static OSKIT_COMDECL_U afile_addref(oskit_absio_t *io)
{
    struct gfile *file;

    if (!io || io->ops != &file_absio_ops)
	    return OSKIT_E_INVALIDARG;

    file = (struct gfile *) ((char *) io - offsetof(struct gfile, f));

    return file_addref(&file->filei);
}

static OSKIT_COMDECL_U afile_release(oskit_absio_t *io)
{
    struct gfile *file;

    if (!io || io->ops != &file_absio_ops)
	    return OSKIT_E_INVALIDARG;

    file = (struct gfile *) ((char *) io - offsetof(struct gfile, f));

    /* may be file_release or dir_release */
    return oskit_file_release(&file->filei);
}


static OSKIT_COMDECL afile_read(oskit_absio_t *io, void *buf,
			       oskit_off_t offset, oskit_size_t amount,
			       oskit_size_t *out_actual)
{
    oskit_error_t ferror;
    struct proc *p;
    struct gfile *file;
    int error, aresid;

    if (!io || io->ops != &file_absio_ops)
	    return OSKIT_E_INVALIDARG;

    file = (struct gfile *) ((char *) io - offsetof(struct gfile, f));
    if (!file->f.count)
	    return OSKIT_E_INVALIDARG;

    if (file->f.vp->v_type != VREG && file->f.vp->v_type != VDIR)
	    return OSKIT_E_NOTIMPL;

    ferror = getproc(&p);
    if (ferror)
	    return ferror;

    VOP_LOCK(file->f.vp);
    error = VOP_ACCESS(file->f.vp, VREAD, p->p_ucred, p);
    VOP_UNLOCK(file->f.vp);
    if (error)
    {
	prfree(p);
	return errno_to_oskit_error(error);
    }

    error = vn_rdwr(UIO_READ, file->f.vp,
		    buf, amount, offset, UIO_SYSSPACE,
		    0, p->p_ucred, &aresid, p);
    prfree(p);
    *out_actual = amount - aresid;
    return errno_to_oskit_error(error);
}


static OSKIT_COMDECL afile_write(oskit_absio_t *io, const void *buf,
				oskit_off_t offset, oskit_size_t amount,
				oskit_size_t *out_actual)
{
    oskit_error_t ferror;
    struct proc *p;
    struct gfile *file;
    int error, aresid;

    if (!io || io->ops != &file_absio_ops)
	    return OSKIT_E_INVALIDARG;

    file = (struct gfile *) ((char *) io - offsetof(struct gfile, f));
    if (!file->f.count)
	    return OSKIT_E_INVALIDARG;

    if (file->f.vp->v_type != VREG)
	    return OSKIT_E_NOTIMPL;

    ferror = getproc(&p);
    if (ferror)
	    return ferror;

    VOP_LOCK(file->f.vp);
    if ((error = vn_writechk(file->f.vp)) != 0 ||
	(error = VOP_ACCESS(file->f.vp, VWRITE, p->p_ucred, p)) != 0)
    {
        VOP_UNLOCK(file->f.vp);
	prfree(p);
	return errno_to_oskit_error(error);
    }
    VOP_UNLOCK(file->f.vp);

    error = vn_rdwr(UIO_WRITE, file->f.vp,
		    (caddr_t)buf, amount, offset, UIO_SYSSPACE,
		    0, p->p_ucred, &aresid, p);
    prfree(p);
    *out_actual = amount - aresid;
    return errno_to_oskit_error(error);
}


static OSKIT_COMDECL afile_get_size(oskit_absio_t *io, oskit_off_t *out_size)
{
    struct oskit_stat stats;
    struct gfile *file;
    oskit_error_t ferror;

    if (!io || io->ops != &file_absio_ops)
	    return OSKIT_E_INVALIDARG;

    file = (struct gfile *) ((char *) io - offsetof(struct gfile, f));
    if (!file->f.count)
	    return OSKIT_E_INVALIDARG;

    ferror = oskit_file_stat(&file->filei, &stats);
    if (ferror)
	    return ferror;

    *out_size = stats.size;
    return 0;
}


static OSKIT_COMDECL afile_set_size(oskit_absio_t *io, oskit_off_t new_size)
{
    struct gfile *file;
    struct oskit_stat stats;
    oskit_u32_t mask;

    if (!io || io->ops != &file_absio_ops)
	    return OSKIT_E_INVALIDARG;

    file = (struct gfile *) ((char *) io - offsetof(struct gfile, f));
    if (!file->f.count)
	    return OSKIT_E_INVALIDARG;

    mask = OSKIT_STAT_SIZE;
    stats.size = new_size;
    return oskit_file_setstat(&file->filei, mask, &stats);
}


static struct oskit_absio_ops file_absio_ops = {
    afile_query,
    afile_addref,
    afile_release,
    (void *)0,			/* slot reserved for getblocksize */
    afile_read,
    afile_write,
    afile_get_size,
    afile_set_size
};



/*
 * oskit_dir methods; we inherit all the file methods above
 */

static OSKIT_COMDECL dir_query(oskit_dir_t *d,
			      const struct oskit_guid *iid,
			      void **out_ihandle)
{
    struct gdir *dir;

    dir = (struct gdir*)d;
    if (dir == NULL)
	    panic("%s:%d: null dir", __FILE__, __LINE__);
    if (dir->f.count == 0)
	    panic("%s:%d: bad count", __FILE__, __LINE__);

    if (memcmp(iid, &oskit_dir_iid, sizeof(*iid)) == 0) {
	*out_ihandle = &dir->diri;
	++dir->f.count;
	return 0;
    }

    return file_query((oskit_file_t *) d, iid, out_ihandle);
}


static OSKIT_COMDECL_U dir_release(oskit_dir_t *d)
{
    struct gdir *dir;
    int rc;
    unsigned newcount;

    dir = (struct gdir*)d;
    if (dir == NULL)
	    panic("%s:%d: null dir", __FILE__, __LINE__);
    if (dir->f.count == 0)
	    panic("%s:%d: bad count", __FILE__, __LINE__);

    newcount = --dir->f.count;
    if (newcount == 0)
    {
	int ferror;
	struct proc *p;

	/* this is the difference between this method and file_release */
	if (dir->virtual && dir->parent)
		oskit_dir_release(dir->parent);

	oskit_filesystem_release(&dir->f.fs->fsi);

	rc = hashtab_remove(vptab, (hashtab_key_t)dir->f.vp, 0, 0);
	assert(rc == HASHTAB_SUCCESS);

	ferror = getproc(&p);
	assert(!ferror);
	vrele(dir->f.vp);
	prfree(p);
	free(dir,M_TEMP);
    }

    return newcount;
}


static OSKIT_COMDECL vp_to_oskit_file(struct gfilesystem *fs,
				    struct vnode *vp,
				    oskit_file_t **out_file)
{
	struct gfile *file;
	oskit_file_t *f;

	file = hashtab_search(vptab, (hashtab_key_t) vp);

	if (file) {
		/* gfile exists for vnode */
		f = &file->filei;
		oskit_file_addref(f);
	}
	else
	{
		/* no gfile for vnode; create a new one */
		f = (oskit_file_t *) gfile_create(fs, vp);
		if (!f)
			return OSKIT_ENOMEM;
	}

	*out_file = f;
	return 0;
}


static OSKIT_COMDECL dir_lookup(oskit_dir_t *d, const char *name,
			       oskit_file_t **out_file)
{
    struct gdir *dir;
    oskit_error_t ferror;
    struct proc *p;
    int error;
    struct nameidata nd;
    struct vnode *dp;

    dir = (struct gdir *)d;

    if (!dir || !dir->f.count)
	    return OSKIT_E_INVALIDARG;

    if (name[0] == '\0') {
	    /*
	     * A lookup of "" refers to the directory itself.
	     */
	    oskit_dir_addref(d);
	    *out_file = (oskit_file_t *) d;
	    return 0;
    }

    /* no more than one component at a time */
    if (strchr (name, '/') != NULL)
	    return OSKIT_E_INVALIDARG;

    ferror = getproc(&p);
    if (ferror)
	    return ferror;

    dp = dir->f.vp;

    if (dir->virtual && !strcmp(name, ".."))
    {
	/* use logical parent */

	/* verify search access to this directory */
	VOP_LOCK(dp);
	error = VOP_ACCESS(dp, VEXEC, p->p_ucred, p);
	VOP_UNLOCK(dp);
	prfree(p);
	if (error)
		return errno_to_oskit_error(error);

	if (dir->parent)
		/* use parent directory provided by prior reparent */
		*out_file = (oskit_file_t *) dir->parent;
	else
		/* virtual root directory; use self as logical parent */
		*out_file = (oskit_file_t *) dir;

	oskit_file_addref(*out_file);
	return 0;
    }

    /*
     * is this a physical root in which we try to look up ".."?
     *
     * NOTE: since namei/lookup handles that case special anyway, we
     * might as well short-cut it. Else we would need to worry about
     * rootvnode (which doesn't really have a meaning here) and
     * p->p_fd->fd_rdir
     */
    if ((dp->v_flag & VROOT) && !strcmp(name, "..")) {
	*out_file = (oskit_file_t *) dir;
	oskit_file_addref(*out_file);
	prfree(p);
	return 0;
    }

    p->p_fd->fd_cdir = dp;
    NDINIT(&nd, LOOKUP, NOFOLLOW, UIO_SYSSPACE, (char *)name, p);
    error = namei(&nd);
    if (error) {
	    prfree(p);
	    return errno_to_oskit_error(error);
    }

    ferror = vp_to_oskit_file(dir->f.fs, nd.ni_vp, out_file);
    vrele(nd.ni_vp);			/* need a curproc for this */
    prfree(p);
    return ferror;
}


static OSKIT_COMDECL dir_create(oskit_dir_t *d, const char *name,
			       oskit_bool_t excl, oskit_mode_t mode,
			       oskit_file_t **out_file)
{
    struct gdir *dir;
    oskit_error_t ferror;
    struct proc *p;
    int error, cmode, flags;
    struct nameidata nd;

    dir = (struct gdir *)d;

    if (!dir || !dir->f.count)
	    return OSKIT_E_INVALIDARG;

    /* no more than one component at a time */
    if (strchr (name, '/') != NULL)
	    return OSKIT_E_INVALIDARG;

    ferror = getproc(&p);
    if (ferror)
	    return ferror;

    cmode = (mode & ALLPERMS) &~ S_ISTXT;

    flags = O_CREAT;
    if (excl)
	    flags |= O_EXCL;

    p->p_fd->fd_cdir = dir->f.vp;
    NDINIT(&nd, CREATE, NOFOLLOW | LOCKLEAF, UIO_SYSSPACE, (char*)name, p);
    if ((error = vn_open(&nd, flags, cmode)) != 0) {
	if (error == ERESTART)
		error = EINTR;
	prfree(p);
	return errno_to_oskit_error(error);
    }


    ferror = vp_to_oskit_file(dir->f.fs, nd.ni_vp, out_file);
    vput(nd.ni_vp);			/* need curproc for this */
    prfree(p);
    return ferror;
}


static OSKIT_COMDECL dir_link(oskit_dir_t *d, char *name,
			     oskit_file_t *f)
{
    struct gfile *file;
    struct gdir *dir, *dir2;
    oskit_error_t ferror;
    struct proc *p;
    int error;
    struct nameidata nd;
    struct vnode *vp;
    const char *cp;

    dir = (struct gdir *)d;
    if (!dir || !dir->f.count)
	    return OSKIT_E_INVALIDARG;

    if (f->ops == (struct oskit_file_ops *) &dir_ops)
    {
	dir2 = (struct gdir *)f;
	if (!dir2 || !dir2->f.count)
		return OSKIT_E_INVALIDARG;
	vp = dir2->f.vp;
    }
    else if (f->ops == &file_ops)
    {
	file = (struct gfile *)f;
	if (!file || !file->f.count)
		return OSKIT_E_INVALIDARG;
	vp = file->f.vp;
    }
    else
    {
	return OSKIT_EXDEV;
    }

    if (vp->v_type == VLNK)
	    return OSKIT_E_NOTIMPL;

    /* no more than one component at a time */
    cp = (const char *) name;
    while (*cp && *cp != '/')
	    cp++;
    if (*cp)
	    return OSKIT_E_INVALIDARG;

    ferror = getproc(&p);
    if (ferror)
	    return ferror;

    /* XXX:  cannot use sys_link; it expects the path of source file */
    p->p_fd->fd_cdir = dir->f.vp;
    NDINIT(&nd, CREATE, LOCKPARENT, UIO_SYSSPACE, name, p);
    error = namei(&nd);
    if (error)
    {
	prfree(p);
	return errno_to_oskit_error(error);
    }

    if (nd.ni_vp) {
	VOP_ABORTOP(nd.ni_dvp, &nd.ni_cnd);
	if (nd.ni_dvp == nd.ni_vp)
		vrele(nd.ni_dvp);
	else
		vput(nd.ni_dvp);
	vrele(nd.ni_vp);
	prfree(p);
	return errno_to_oskit_error(EEXIST);
    }
    VOP_LEASE(nd.ni_dvp, p, p->p_ucred, LEASE_WRITE);
    VOP_LEASE(vp, p, p->p_ucred, LEASE_WRITE);
    error = VOP_LINK(nd.ni_dvp, vp, &nd.ni_cnd);
    prfree(p);
    if (error)
	    return errno_to_oskit_error(error);

    return 0;
}


static OSKIT_COMDECL dir_unlink(oskit_dir_t *d, const char *name)
{
    struct gdir *dir;
    oskit_error_t ferror;
    struct sys_unlink_args args;
    struct proc *p;
    int error;
    register_t retval;
    const char *cp;

    dir = (struct gdir *)d;
    if (!dir || !dir->f.count)
	    return OSKIT_E_INVALIDARG;

    /* no more than one component at a time */
    cp = (const char *) name;
    while (*cp && *cp != '/')
	    cp++;
    if (*cp)
	    return OSKIT_E_INVALIDARG;

    ferror = getproc(&p);
    if (ferror)
	    return ferror;
    p->p_fd->fd_cdir = dir->f.vp;
    SCARG(&args, path) = (char *)name;
    error = sys_unlink(p, &args, &retval);
    prfree(p);
    return errno_to_oskit_error(error);
}


static OSKIT_COMDECL dir_rename(oskit_dir_t *old_d, char *old_name,
			       oskit_dir_t *new_d, char *new_name)
{
    struct gdir *old_dir, *new_dir;
    oskit_error_t ferror;
    struct proc *p;
    int error;
    char *path;
    const char *cp;
    struct sys_rename_args args;
    register_t retval;

    old_dir = (struct gdir *)old_d;
    if (!old_dir || !old_dir->f.count)
	    return OSKIT_E_INVALIDARG;

    if (new_d->ops != &dir_ops)
	    return OSKIT_EXDEV;

    new_dir = (struct gdir *)new_d;
    if (!new_dir || !new_dir->f.count)
	    return OSKIT_E_INVALIDARG;

    /* no more than one component at a time */
    cp = (const char *) old_name;
    while (*cp && *cp != '/')
	    cp++;
    if (*cp)
	    return OSKIT_E_INVALIDARG;
    cp = (const char *) new_name;
    while (*cp && *cp != '/')
	    cp++;
    if (*cp)
	    return OSKIT_E_INVALIDARG;

    /*
     * sys_rename assumes that both names are looked up
     * relative to the same (rdir, cdir) pair.
     * However, we need the lookup of old_name to be
     * relative to old_dir and the lookup of new_name to
     * be relative to new_dir.  So, we use old_dir
     * as rdir, new_dir as cdir, and prepend a /
     * to old_name.  Sigh.
     */
    path = (char *) malloc(strlen(old_name)+2, M_TEMP, M_WAITOK);
    if (!path)
	    return OSKIT_ENOMEM;
    sprintf(path, "/%s", old_name);

    ferror = getproc(&p);
    if (ferror)
    {
	FREE(path,M_TEMP);
	return ferror;
    }
    p->p_fd->fd_rdir = old_dir->f.vp;
    p->p_fd->fd_cdir = new_dir->f.vp;
    SCARG(&args, from) = path;
    SCARG(&args, to) = new_name;
    error = sys_rename(p, &args, &retval);
    FREE(path,M_TEMP);
    prfree(p);
    return errno_to_oskit_error(error);
}


static OSKIT_COMDECL dir_mkdir(oskit_dir_t *d, const char *name,
			      oskit_mode_t mode)
{
    struct gdir *dir;
    oskit_error_t ferror;
    struct proc *p;
    int error;
    struct sys_mkdir_args args;
    const char *cp;
    register_t retval;


    dir = (struct gdir *)d;
    if (!dir || !dir->f.count)
	    return OSKIT_E_INVALIDARG;

    /* no more than one component at a time */
    cp = (const char *) name;
    while (*cp && *cp != '/')
	    cp++;
    if (*cp)
	    return OSKIT_E_INVALIDARG;

    ferror = getproc(&p);
    if (ferror)
	    return ferror;
    p->p_fd->fd_cdir = dir->f.vp;
    SCARG(&args, path) = (char *) name;
    SCARG(&args, mode) = (int)mode;
    error = sys_mkdir(p,&args,&retval);
    prfree(p);
    return errno_to_oskit_error(error);
}


static OSKIT_COMDECL dir_rmdir(oskit_dir_t *d, const char *name)
{
    struct gdir *dir;
    oskit_error_t ferror;
    struct proc *p;
    struct sys_rmdir_args args;
    const char *cp;
    int error;
    register_t retval;

    dir = (struct gdir *)d;
    if (!dir || !dir->f.count)
	    return OSKIT_E_INVALIDARG;

    /* no more than one component at a time */
    cp = (const char *) name;
    while (*cp && *cp != '/')
	    cp++;
    if (*cp)
	    return OSKIT_E_INVALIDARG;

    ferror = getproc(&p);
    if (ferror)
	    return ferror;
    p->p_fd->fd_cdir = dir->f.vp;
    SCARG(&args, path) = (char *) name;
    error = sys_rmdir(p, &args, &retval);
    prfree(p);
    return errno_to_oskit_error(error);
}


static char dirbuf[DEV_BSIZE];

static OSKIT_COMDECL dir_getdirentries(oskit_dir_t *d, oskit_u32_t *inout_ofs,
				      oskit_u32_t nentries,
				      struct oskit_dirents **out_dirents)
{
    struct gdir *dir;
    oskit_error_t ferror;
    struct proc *p;
    struct uio auio;
    struct iovec aiov;
    struct dirent *dp, *edp;
    int error, eofflag, actual, count, dircount;
    oskit_error_t rc;
    struct internal_dirent *dirents;

    dir = (struct gdir *)d;
    if (!dir || !dir->f.count || !out_dirents || !inout_ofs)
	    return OSKIT_E_INVALIDARG;

    ferror = getproc(&p);
    if (ferror)
	    return ferror;

    VOP_LOCK(dir->f.vp);
    error = VOP_ACCESS(dir->f.vp, VREAD, p->p_ucred, p);
    VOP_UNLOCK(dir->f.vp);
    if (error) {
	    prfree(p);
	    return errno_to_oskit_error(error);
    }

    dircount = 0;
    MALLOC(dirents, struct internal_dirent *,
	   sizeof(struct internal_dirent)*nentries,M_TEMP,M_WAITOK);
    assert(dirents);

    error = 0;
    while (dircount < nentries)
    {
	aiov.iov_base = dirbuf;
	aiov.iov_len = sizeof(dirbuf);
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_rw = UIO_READ;
	auio.uio_segflg = UIO_SYSSPACE;
	auio.uio_resid = sizeof(dirbuf);
	auio.uio_offset = *inout_ofs;
	auio.uio_procp = p;
	VOP_LOCK(dir->f.vp);
	error = VOP_READDIR(dir->f.vp, &auio, p->p_ucred, &eofflag,
			    (u_long *)0, 0);
	VOP_UNLOCK(dir->f.vp);
	*inout_ofs = auio.uio_offset;
	actual = sizeof(dirbuf) - auio.uio_resid;

	if (error || actual == 0)
		/* error or end-of-file */
		break;

	/*
	 * First count them up in case we need to make the dirents
	 * array bigger.
	 */
	count = 0;
	edp = (struct dirent *)&dirbuf[actual];
	for (dp = (struct dirent *)dirbuf; dp < edp; )
	{
	    if (dp->d_reclen <= 0)
		    break;

	    count++;
	    
	    dp = (struct dirent *) ((char *)dp + dp->d_reclen);
	}

	/*
	 * If there are too many, increase the dirents array in one chunk.
	 */
	if (dircount + count > nentries)
	{
	    int newcount = dircount + count;
	    struct internal_dirent *ptmp;

	    MALLOC(ptmp, struct internal_dirent *,
		   sizeof(struct internal_dirent)*newcount, M_TEMP, M_WAITOK);
	    assert(ptmp);

	    memcpy(ptmp, dirents,
		   sizeof(struct internal_dirent) * dircount);

	    FREE(dirents, M_TEMP);
	    dirents = ptmp;
	}

	/*
	 * Finally, copy them out.
	 */
	edp = (struct dirent *)&dirbuf[actual];
	for (dp = (struct dirent *)dirbuf; dp < edp; )
	{
	    if (dp->d_reclen <= 0)
		    break;

	    dircount++;

	    MALLOC(dirents[dircount-1].name,
		   char *, sizeof(char)*(dp->d_namlen+1),
		   M_TEMP, M_WAITOK);
	    assert(dirents[dircount-1].name);
	    strncpy(dirents[dircount-1].name, dp->d_name,dp->d_namlen+1);

	    dirents[dircount-1].ino = dp->d_fileno;
	    dirents[dircount-1].namelen = dp->d_namlen;

	    dp = (struct dirent *) ((char *)dp + dp->d_reclen);
	}
    }

    /*
     * Create the dirents COM object first to make sure we can.
     */
    rc = fs_dirents_create(dirents, dircount, out_dirents);
    assert(!rc);

    prfree(p);
    return errno_to_oskit_error(error);
}


static OSKIT_COMDECL dir_mknod(oskit_dir_t *d, char *name,
			      oskit_mode_t mode, oskit_dev_t dev)
{
    struct gdir *dir;
    struct sys_mknod_args args;
    oskit_error_t ferror;
    struct proc *p;
    const char *cp;
    int error;
    register_t retval;

    dir = (struct gdir *)d;
    if (!dir || !dir->f.count)
	    return OSKIT_E_INVALIDARG;

    /* no more than one component at a time */
    cp = (const char *) name;
    while (*cp && *cp != '/')
	    cp++;
    if (*cp)
	    return OSKIT_E_INVALIDARG;

    ferror = getproc(&p);
    if (ferror)
	    return ferror;

    p->p_fd->fd_cdir = dir->f.vp;
    SCARG(&args, path) = (char *)name;
    SCARG(&args, mode) = (int)mode;
    SCARG(&args, dev) = (int)dev;
    error = sys_mknod(p, &args, &retval);
    prfree(p);
    return errno_to_oskit_error(error);
}


static OSKIT_COMDECL dir_symlink(oskit_dir_t *d, char *link_name,
				char *dest_name)
{
    struct gdir *dir;
    oskit_error_t ferror;
    struct proc *p;
    struct sys_symlink_args args;
    const char *cp;
    int error;
    register_t retval;

    dir = (struct gdir *)d;
    if (!dir || !dir->f.count)
	    return OSKIT_E_INVALIDARG;

    /* no more than one component at a time */
    cp = (const char *) link_name;
    while (*cp && *cp != '/')
	    cp++;
    if (*cp)
	    return OSKIT_E_INVALIDARG;

    ferror = getproc(&p);
    if (ferror)
	    return ferror;

    p->p_fd->fd_cdir = dir->f.vp;
    SCARG(&args, path) = dest_name;
    SCARG(&args, link) = link_name;
    error = sys_symlink(p, &args, &retval);
    prfree(p);
    return errno_to_oskit_error(error);
}


static OSKIT_COMDECL dir_reparent(oskit_dir_t *d, oskit_dir_t *parent,
				 oskit_dir_t **out_dir)
{
    struct gdir *dir, *new_dir;

    dir = (struct gdir *)d;
    if (!dir || !dir->f.count)
	    return OSKIT_E_INVALIDARG;

    /* XXX:  any Unix access controls on reparent? */

    /*
     * We do NOT use gdir_create for virtual directory objects;
     * this would pose a problem for the vp -> dir mapping,
     * which assumes a single dir for every vnode.  Since
     * the vp -> dir mapping is ONLY used to obtain the
     * physical directory object for a physical directory
     * vnode, this isn't a problem.
     */

    MALLOC(new_dir,struct gdir*,sizeof(struct gdir),M_TEMP,M_WAITOK);
    if (!new_dir)
	    return OSKIT_ENOMEM;

    new_dir->diri.ops = &dir_ops;
    new_dir->f.count = 1;
    new_dir->f.fs = dir->f.fs;
    dir->f.vp = dir->f.vp;
    dir->virtual = TRUE;
    dir->parent = parent;
    VREF(dir->f.vp);
    oskit_filesystem_addref(&dir->f.fs->fsi);
    if (parent)
	    oskit_dir_addref(parent);

    *out_dir = &new_dir->diri;
    return 0;
}


static struct oskit_dir_ops dir_ops = {
    dir_query,
    file_addref,
    dir_release,
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
 * oskit_dirents_t COM object implementation.
 */
struct fs_dirents {
	oskit_dirents_t		direntsi;	/* COM dirents interface */
	int			count;		/* Reference count */
	internal_dirent_t       *entries;	/* Array of entries */
	int			dircount;	/* Number of entries */
	int			index;		/* Current index pointer */
};

/*
 * Methods.
 */
static struct oskit_dirents_ops fs_dirents_ops;

static OSKIT_COMDECL
fs_dirents_query(oskit_dirents_t *d,
		 const oskit_iid_t *iid, void **out_ihandle)
{
	struct fs_dirents *fsdirents = (struct fs_dirents *) d;
	assert(fsdirents->count);

        if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
            memcmp(iid, &oskit_dirents_iid, sizeof(*iid)) == 0) {
                *out_ihandle = fsdirents;
                ++fsdirents->count;
                return 0;
        }

        *out_ihandle = NULL;
        return OSKIT_E_NOINTERFACE;
}

static OSKIT_COMDECL_U
fs_dirents_addref(oskit_dirents_t *d)
{
	struct fs_dirents *fsdirents = (struct fs_dirents *) d;
	assert(fsdirents->count);
	
	return ++fsdirents->count;
}

static OSKIT_COMDECL_U
fs_dirents_release(oskit_dirents_t *d)
{
	struct fs_dirents *fsdirents = (struct fs_dirents *) d;
	int i;
	
	assert(fsdirents->count);

	if (--fsdirents->count)
		return fsdirents->count;

	for (i = 0; i < fsdirents->dircount; i++)
		FREE(fsdirents->entries[i].name, M_TEMP);

	FREE(fsdirents->entries, M_TEMP);
	FREE(fsdirents, M_TEMP);	
	return 0;
}

static OSKIT_COMDECL
fs_dirents_getcount(oskit_dirents_t *d, int *out_count)
{
	struct fs_dirents *fsdirents = (struct fs_dirents *) d;
	assert(fsdirents->count);

	*out_count = fsdirents->dircount;

	return 0;
}

static OSKIT_COMDECL
fs_dirents_getnext(oskit_dirents_t *d, oskit_dirent_t *dirent)
{
	struct fs_dirents	*fsdirents = (struct fs_dirents *) d;
	struct internal_dirent  *idirent;
	
	assert(fsdirents->count);

	if (fsdirents->index >= fsdirents->dircount)
		return OSKIT_EWOULDBLOCK;

	idirent = &fsdirents->entries[fsdirents->index];

	if (dirent->namelen < idirent->namelen + 1)
		return OSKIT_ENAMETOOLONG;

	dirent->ino     = idirent->ino;
	dirent->namelen = idirent->namelen;
	strcpy(dirent->name, idirent->name);
	
	fsdirents->index++;	
	return 0;
}

static OSKIT_COMDECL
fs_dirents_rewind(oskit_dirents_t *d)
{
	struct fs_dirents *fsdirents = (struct fs_dirents *) d;
	assert(fsdirents->count);

	fsdirents->index = 0;
	return 0;
}

static struct oskit_dirents_ops fs_dirents_ops = {
	fs_dirents_query,
	fs_dirents_addref,
	fs_dirents_release,
	fs_dirents_getcount,
	fs_dirents_getnext,
	fs_dirents_rewind,
};

/*
 * Create a oskit_dirents_t COM object from an array of oskit_dirent_t
 * entries.
 */
static oskit_error_t
fs_dirents_create(internal_dirent_t *entries,
		  int count, oskit_dirents_t **out_dirents)
{
        struct fs_dirents *fsdirents;
 
	MALLOC(fsdirents, struct fs_dirents *,
	       sizeof(struct fs_dirents),M_TEMP,M_WAITOK);

        if (fsdirents == NULL)
                return OSKIT_ENOMEM;

        fsdirents->count        = 1;
        fsdirents->dircount     = count;
        fsdirents->entries      = entries;
        fsdirents->index        = 0;
	fsdirents->direntsi.ops = &fs_dirents_ops;

	*out_dirents = &fsdirents->direntsi;

	return 0;
}

#if 0
/* This is just for debugging. */
oskit_error_t
fs_netbsd_dump_blocks(oskit_file_t *f)
{
    struct gfile *file;
    struct proc *p;
    oskit_error_t ferror;
    int i;
    int error;
    daddr_t db;
    
    file = (struct gfile *)f;

    if (!file || !file->f.count)
	    return OSKIT_E_INVALIDARG;
    
    ferror = getproc(&p);
    if (ferror)
	    return ferror;

    error = 0;
    for (i = 0; ; i++) {
	    error = VOP_BMAP(file->f.vp, i, NULL, &db, NULL);
	    if (error || db == -1)
		    break;
	    printf("%d -> %d\n", i, db);
    }

    prfree(p);
    return errno_to_oskit_error(error);
}
#endif

