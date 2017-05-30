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

#define offsetof(type, member) ((size_t)(&((type *)0)->member))

/*
 * oskit_openfile methods; per-open operations
 */


/*
 * description-based helper functions for oskit_openfile methods
 */

static oskit_error_t fp_read(struct file *fp, void *buf, oskit_u32_t len,
			    oskit_u32_t *out_actual)
{
    oskit_error_t ferror;
    struct proc *p;
    struct uio auio;
    struct iovec aiov;
    long cnt, error = 0;
    
    if ((fp->f_flag & FREAD) == 0)
	    return OSKIT_EBADF;

    ferror = getproc(&p);
    if (ferror)
	    return ferror;

    aiov.iov_base = (caddr_t)buf;
    aiov.iov_len = len;
    auio.uio_iov = &aiov;
    auio.uio_iovcnt = 1;
    auio.uio_resid = len;
    auio.uio_rw = UIO_READ;
    auio.uio_segflg = UIO_SYSSPACE;
    auio.uio_procp = p;
    if (auio.uio_resid < 0) {
	    prfree(p);
	    return OSKIT_EINVAL;
    }
    cnt = len;
    error = (*fp->f_ops->fo_read)(fp, &auio, fp->f_cred);
    if (error)
	    if (auio.uio_resid != cnt && (error == ERESTART ||
					  error == EINTR || error == EWOULDBLOCK))
		    error = 0;
    cnt -= auio.uio_resid;
    *out_actual = cnt;
    prfree(p);
    return errno_to_oskit_error(error);    
}


static oskit_error_t fp_write(struct file *fp, const void *buf,
			     oskit_u32_t len, oskit_u32_t *out_actual)
{
    oskit_error_t ferror;
    struct proc *p;
    struct uio auio;
    struct iovec aiov;
    long cnt, error = 0;
    
    if ((fp->f_flag & FWRITE) == 0)
	    return OSKIT_EBADF;

    ferror = getproc(&p);
    if (ferror)
	    return ferror;

    aiov.iov_base = (char *)buf;	/* XXX kills const */
    aiov.iov_len = len;
    auio.uio_iov = &aiov;
    auio.uio_iovcnt = 1;
    auio.uio_resid = len;
    auio.uio_rw = UIO_WRITE;
    auio.uio_segflg = UIO_SYSSPACE;
    auio.uio_procp = p;
    if (auio.uio_resid < 0) {
	    prfree(p);
	    return OSKIT_EINVAL;
    }
    cnt = len;
    error = (*fp->f_ops->fo_write)(fp, &auio, fp->f_cred);
    if (error) {
	if (auio.uio_resid != cnt && (error == ERESTART ||
				      error == EINTR || error == EWOULDBLOCK))
		error = 0;
#ifndef OSKIT
	if (error == EPIPE)
		psignal(p, SIGPIPE);
#endif
    }
    cnt -= auio.uio_resid;
    *out_actual = cnt;
    prfree(p);
    return errno_to_oskit_error(error);    
}


static oskit_error_t fp_seek(struct file *fp, oskit_s64_t ofs,
			    oskit_seek_t whence, oskit_u64_t *out_newpos)
{
    oskit_error_t ferror;
    struct proc *p;
    struct vattr vattr;
    int error;
    
    switch (whence) {
      case OSKIT_SEEK_CUR:
	fp->f_offset += ofs;
	break;
      case OSKIT_SEEK_END:
	ferror = getproc(&p);
	if (ferror)
		return ferror;

	error = VOP_GETATTR((struct vnode *)fp->f_data, &vattr,
			    p->p_ucred, p);

	prfree(p);

	if (error)
		return errno_to_oskit_error(error);
	fp->f_offset = ofs + vattr.va_size;
	break;
      case OSKIT_SEEK_SET:
	fp->f_offset = ofs;
	break;
      default:
	return OSKIT_EINVAL;
    }
    *out_newpos = fp->f_offset;
    return (0);
}


/*
 * oskit_openfile internal constructor
 */

static struct oskit_openfile_ops ofile_ops;
static struct oskit_absio_ops ofile_absio_ops;

struct gopenfile *gopenfile_create(struct gfile *file, struct file *fp)
{
    struct gopenfile *ofile;
    
    MALLOC(ofile,struct gopenfile*,sizeof(struct gopenfile),M_TEMP,M_WAITOK);
    if (!ofile)
	    return NULL;

    ofile->ofilei.ops = &ofile_ops; 
    ofile->absioi.ops = &ofile_absio_ops; 
    ofile->count = 1;
    ofile->file = file;
    ofile->fp = fp;
    oskit_file_addref(&file->filei);	

    return ofile;
}


/*
 * oskit_openfile methods
 */

static OSKIT_COMDECL ofile_query(oskit_openfile_t *f,
				const struct oskit_guid *iid,
				void **out_ihandle)
{
    struct gopenfile *ofile;

    ofile = (struct gopenfile*)f;
    if (ofile == NULL)
	    panic("%s:%d: null file", __FILE__, __LINE__);
    if (ofile->count == 0)
	    panic("%s:%d: bad count", __FILE__, __LINE__);
    
    if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
	memcmp(iid, &oskit_stream_iid, sizeof(*iid)) == 0 ||
	memcmp(iid, &oskit_openfile_iid, sizeof(*iid)) == 0) {
	*out_ihandle = &ofile->ofilei;
	++ofile->count;
	return 0;
    }

    if (memcmp(iid, &oskit_absio_iid, sizeof(*iid)) == 0) {
	*out_ihandle = &ofile->absioi;
	++ofile->count;
	return 0;
    }

    *out_ihandle = NULL;
    return OSKIT_E_NOINTERFACE;    
}


static OSKIT_COMDECL_U ofile_addref(oskit_openfile_t *f)
{
    struct gopenfile *ofile;

    ofile = (struct gopenfile*)f;
    if (ofile == NULL)
	    panic("%s:%d: null file", __FILE__, __LINE__);
    if (ofile->count == 0)
	    panic("%s:%d: bad count", __FILE__, __LINE__);

    return ++ofile->count;
}


static OSKIT_COMDECL_U ofile_release(oskit_openfile_t *f)
{
    struct gopenfile *ofile;
    unsigned newcount;
    
    ofile = (struct gopenfile*)f; 
    if (ofile == NULL)
	    panic("%s:%d: null file", __FILE__, __LINE__);
    if (ofile->count == 0)
	    panic("%s:%d: bad count", __FILE__, __LINE__);    

    newcount = --ofile->count;
    if (newcount == 0)
    {
	    oskit_error_t ferror;
	    struct proc *p;

	    oskit_file_release(&ofile->file->filei);

	    ferror = getproc(&p);
	    assert(!ferror);	/* XXX */
	    closef(ofile->fp, NULL);
	    prfree(p);

	    free(ofile,M_TEMP);
    }

    return newcount;
}


static OSKIT_COMDECL ofile_read(oskit_openfile_t *f, void *buf, oskit_u32_t len,
			       oskit_u32_t *out_actual)
{
    struct gopenfile *ofile;
    

    ofile = (struct gopenfile *)f;
    if (!ofile || !ofile->count)
	    return OSKIT_E_INVALIDARG;

    return fp_read(ofile->fp,buf,len,out_actual);
}


static OSKIT_COMDECL ofile_write(oskit_openfile_t *f, const void *buf,
				oskit_u32_t len, oskit_u32_t *out_actual)
{
    struct gopenfile *ofile;
    

    ofile = (struct gopenfile *)f;
    if (!ofile || !ofile->count)
	    return OSKIT_E_INVALIDARG;
    
    return fp_write(ofile->fp,buf,len,out_actual);    
}


static OSKIT_COMDECL ofile_seek(oskit_openfile_t *f, oskit_s64_t ofs,
			       oskit_seek_t whence, oskit_u64_t *out_newpos)
{
    struct gopenfile *ofile;
    

    ofile = (struct gopenfile *)f;
    if (!ofile || !ofile->count)
	    return OSKIT_E_INVALIDARG;
    
    return fp_seek(ofile->fp,ofs,whence,out_newpos);
}


static OSKIT_COMDECL ofile_setsize(oskit_openfile_t *f, oskit_u64_t new_size)
{
    struct gopenfile *ofile;
    struct oskit_stat stats;
    oskit_u32_t mask;

    ofile = (struct gopenfile *)f;
    if (!ofile || !ofile->count)
	    return OSKIT_E_INVALIDARG;
    
    mask = OSKIT_STAT_SIZE;
    stats.size = new_size;
    return oskit_file_setstat(&ofile->file->filei, mask, &stats);
}


static OSKIT_COMDECL ofile_copy_to(oskit_openfile_t *f, 
				  oskit_stream_t *dst,
				  oskit_u64_t size,
				  oskit_u64_t *out_read,
				  oskit_u64_t *out_written)
{
    return OSKIT_E_NOTIMPL;    
}


static OSKIT_COMDECL ofile_commit(oskit_openfile_t *f, oskit_u32_t commit_flags)
{
    return OSKIT_E_NOTIMPL;        
}


static OSKIT_COMDECL ofile_revert(oskit_openfile_t *f)
{
    return OSKIT_E_NOTIMPL;            
}


static OSKIT_COMDECL ofile_lock_region(oskit_openfile_t *f,
				      oskit_u64_t offset, oskit_u64_t size,
				      oskit_u32_t lock_type)
{
    return OSKIT_E_NOTIMPL;            
}


static OSKIT_COMDECL ofile_unlock_region(oskit_openfile_t *f,
					oskit_u64_t offset, oskit_u64_t size,
					oskit_u32_t lock_type)
{
    return OSKIT_E_NOTIMPL;                
}


static OSKIT_COMDECL ofile_stat(oskit_openfile_t *f, oskit_stream_stat_t *out_stat,
			       oskit_u32_t stat_flags)
{
    return OSKIT_E_NOTIMPL;                    
}


static OSKIT_COMDECL ofile_clone(oskit_openfile_t *f, oskit_openfile_t **out_stream)
{
    return OSKIT_E_NOTIMPL;                        
}


static OSKIT_COMDECL ofile_getfile(oskit_openfile_t *f, 
				  struct oskit_file **out_file)
{
    struct gopenfile *ofile;
    

    ofile = (struct gopenfile *)f;
    if (!ofile || !ofile->count)
	    return OSKIT_E_INVALIDARG;
 
    oskit_file_addref(&ofile->file->filei);
    *out_file = &ofile->file->filei;

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

static OSKIT_COMDECL afile_query(oskit_absio_t *io,
			       const struct oskit_guid *iid,
			       void **out_ihandle)
{
    struct gopenfile *ofile;

    if (!io || io->ops != &ofile_absio_ops)
	    return OSKIT_E_INVALIDARG;

    ofile = (struct gopenfile *) ((char *) io -
				  offsetof(struct gopenfile, absioi));
    return oskit_openfile_query(&ofile->ofilei,iid,out_ihandle);
}


static OSKIT_COMDECL_U afile_addref(oskit_absio_t *io)
{
    struct gopenfile *ofile;


    if (!io || io->ops != &ofile_absio_ops)
	    return OSKIT_E_INVALIDARG;

    ofile = (struct gopenfile *) ((char *) io -
				  offsetof(struct gopenfile, absioi));
    return oskit_openfile_addref(&ofile->ofilei);
}


static OSKIT_COMDECL_U afile_release(oskit_absio_t *io)
{
    struct gopenfile *ofile;

    if (!io || io->ops != &ofile_absio_ops)
	    return OSKIT_E_INVALIDARG;
    
    ofile = (struct gopenfile *) ((char *) io -
				  offsetof(struct gopenfile, absioi));
    return oskit_openfile_release(&ofile->ofilei);
}


static OSKIT_COMDECL afile_read(oskit_absio_t *io, void *buf,
			       oskit_off_t offset, oskit_size_t amount,
			       oskit_size_t *out_actual)
{
    oskit_error_t ferror;
    struct proc *p;
    struct gopenfile *ofile;
    int error, aresid;

    if (!io || io->ops != &ofile_absio_ops)
	    return OSKIT_E_INVALIDARG;
    
    ofile = (struct gopenfile *) ((char *) io -
				  offsetof(struct gopenfile, absioi));
    if (!ofile->count)
	    return OSKIT_E_INVALIDARG;

    if ((ofile->fp->f_flag & FREAD) == 0)
	    return OSKIT_EBADF;

    ferror = getproc(&p);
    if (ferror)
	    return ferror;

    error = vn_rdwr(UIO_READ, ofile->file->f.vp, 
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
    struct gopenfile *ofile;
    int error, aresid;

    if (!io || io->ops != &ofile_absio_ops)
	    return OSKIT_E_INVALIDARG;
    
    ofile = (struct gopenfile *) ((char *) io -
				  offsetof(struct gopenfile, absioi));
    if (!ofile->count)
	    return OSKIT_E_INVALIDARG;

    if ((ofile->fp->f_flag & FWRITE) == 0)
	    return OSKIT_EBADF;

    ferror = getproc(&p);
    if (ferror)
	    return ferror;

    error = vn_rdwr(UIO_WRITE, ofile->file->f.vp, 
		    (caddr_t)buf, amount, offset, UIO_SYSSPACE,
		    0, p->p_ucred, &aresid, p);    
    prfree(p);
    *out_actual = amount - aresid;
    return errno_to_oskit_error(error);
}


static OSKIT_COMDECL afile_get_size(oskit_absio_t *io, oskit_off_t *out_size)
{
    struct oskit_stat stats;
    struct gopenfile *ofile;
    oskit_error_t ferror;

    if (!io || io->ops != &ofile_absio_ops)
	    return OSKIT_E_INVALIDARG;
    
    ofile = (struct gopenfile *) ((char *) io -
				  offsetof(struct gopenfile, absioi));
    if (!ofile->count)
	    return OSKIT_E_INVALIDARG;
    
    ferror = oskit_file_stat(&ofile->file->filei, &stats);
    if (ferror)
	    return ferror;
    
    *out_size = stats.size;
    return 0;
}


static OSKIT_COMDECL afile_set_size(oskit_absio_t *io, oskit_off_t new_size)
{
    struct gopenfile *ofile;
    struct oskit_stat stats;
    oskit_u32_t mask;

    if (!io || io->ops != &ofile_absio_ops)
	    return OSKIT_E_INVALIDARG;
    
    ofile = (struct gopenfile *) ((char *) io -
				  offsetof(struct gopenfile, absioi));
    if (!ofile->count)
	    return OSKIT_E_INVALIDARG;
    
    mask = OSKIT_STAT_SIZE;
    stats.size = new_size;
    return oskit_file_setstat(&ofile->file->filei, mask, &stats);
}


static struct oskit_absio_ops ofile_absio_ops = {
    afile_query,
    afile_addref,
    afile_release,
    (void *)0,
    afile_read,
    afile_write,
    afile_get_size,
    afile_set_size
};
