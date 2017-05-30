/*
 * Copyright (c) 1997, 1998, 1999 The University of Utah and the Flux Group.
 * 
 * This file is part of the OSKit Linux Glue Libraries, which are free
 * software, also known as "open source;" you can redistribute them and/or
 * modify them under the terms of the GNU General Public License (GPL),
 * version 2, as published by the Free Software Foundation (FSF).
 * 
 * The OSKit is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GPL for more details.  You should have
 * received a copy of the GPL along with the OSKit; see the file COPYING.  If
 * not, write to the FSF, 59 Temple Place #330, Boston, MA 02111-1307, USA.
 */
/*
 * Implements the oskit_openfile_t interface,
 * which also implements oskit_absio.
 */

#include <oskit/fs/openfile.h>
#include <oskit/io/absio.h>

#include <linux/fs.h>
#include <linux/file.h>
#include <linux/malloc.h>

#include "current.h"
#include "errno.h"
#include "types.h"

/* So we don't need <oskit/c/stddef.h>. */
#define	offsetof(type, member)	((size_t)(&((type *)0)->member))

static struct oskit_openfile_ops ofile_ops;
static struct oskit_absio_ops ofile_absio_ops;


/*
 * oskit_openfile_t
 */

static OSKIT_COMDECL
ofile_query(oskit_openfile_t *f,
	    const struct oskit_guid *iid,
	    void **out_ihandle)
{
	gopenfile_t *ofile;

	ofile = (gopenfile_t *)f;
	if (!verify_openfile(ofile)) return OSKIT_E_INVALIDARG;

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

static OSKIT_COMDECL_U
ofile_addref(oskit_openfile_t *f)
{
	gopenfile_t *ofile;

	ofile = (gopenfile_t *)f;
	VERIFY_OR_PANIC(ofile, "openfile");

	return ++ofile->count;
}

/*
 * This needs to undo what oskit_file_open and gopenfile_create do.
 */
static OSKIT_COMDECL_U
ofile_release(oskit_openfile_t *f)
{
	gopenfile_t *ofile;
	unsigned newcount;

	ofile = (gopenfile_t *)f;
	VERIFY_OR_PANIC(ofile, "openfile");

	newcount = --ofile->count;
	if (newcount == 0) {
		oskit_file_release(&ofile->gf->filei);
		fput(ofile->filp); /* dputs for us */
		kfree(ofile->filp);
		kfree(ofile);
	}

	return newcount;
}

/*
 * Common code for reading and writing.
 * Very similar to Linux's fs/read_write.c:sys_read/write.
 * Ends up calling the read or write f_op.
 */
static oskit_error_t
do_rw(int rw, gopenfile_t *ofile, void *buf, oskit_u32_t len,
      oskit_u32_t *out_actual)
{
	struct task_struct ts;
	oskit_error_t err;
	struct inode *inode;
	struct dentry *dentry;
	struct file *filp;
	int lerr;
	loff_t offset;

	/*
	 * Basic validity checks on params should have been done in caller.
	 * We check more semantic things.
	 */
	filp = ofile->filp;
	dentry = filp->f_dentry;
	inode  = dentry->d_inode;
	offset = filp->f_pos;
	if (inode == NULL || !(filp->f_mode & rw))
		return OSKIT_EBADF;
	if (filp->f_op == NULL ||
	    (rw == FMODE_READ && filp->f_op->read == NULL) ||
	    (rw == FMODE_WRITE && filp->f_op->write == NULL))
		return OSKIT_EINVAL;

	/*
	 * Call the appropriate f_op.
	 */
	err = fs_linux_create_current(&ts);
	if (err)
		return err;
	if (rw == FMODE_READ)
		lerr = filp->f_op->read(filp, buf, len, &offset);
	else {
		/*
		 * From Linux's fs/read_write.c:sys_write:
		 *
		 * If data has been written to the file, remove the setuid and
		 * the setgid bits. We do it anyway otherwise there is an
		 * extremely exploitable race - does your OS get it right |->
		 *
		 * Set ATTR_FORCE so it will always be changed.
		 */
		if (!suser() && (inode->i_mode & (S_ISUID | S_ISGID))) {
			struct iattr newattrs;
			/*
			 * Don't turn off setgid if no group execute. This special
			 * case marks candidates for mandatory locking.
			 */
			newattrs.ia_mode = inode->i_mode &
					   ~(S_ISUID | ((inode->i_mode & S_IXGRP) ? S_ISGID : 0));
			newattrs.ia_valid = ATTR_CTIME | ATTR_MODE | ATTR_FORCE;
			notify_change(dentry, &newattrs);
		}

		lerr = filp->f_op->write(filp, buf, len, &offset);
	}
	*out_actual = 0;
	if (lerr >= 0)
		*out_actual = lerr;
	fs_linux_destroy_current();
	return (lerr < 0) ? errno_to_oskit_error(-lerr) : 0;
}

static OSKIT_COMDECL
ofile_read(oskit_openfile_t *f, void *buf, oskit_u32_t len,
	   oskit_u32_t *out_actual)
{
	gopenfile_t *ofile;

	ofile = (gopenfile_t *)f;
	if (!verify_openfile(ofile)) return OSKIT_E_INVALIDARG;

	if (buf == NULL || out_actual == NULL)
		return OSKIT_E_INVALIDARG;

	if (len == 0) {
		*out_actual = 0;
		return 0;
	}

	return do_rw(FMODE_READ, ofile, buf, len, out_actual);
}

static OSKIT_COMDECL
ofile_write(oskit_openfile_t *f, const void *buf,
	    oskit_u32_t len, oskit_u32_t *out_actual)
{
	gopenfile_t *ofile;

	ofile = (gopenfile_t *)f;
	if (!verify_openfile(ofile)) return OSKIT_E_INVALIDARG;

	if (buf == NULL || out_actual == NULL)
		return OSKIT_E_INVALIDARG;

	if (len == 0) {
		*out_actual = 0;
		return 0;
	}

	return do_rw(FMODE_WRITE, ofile, (void *)buf, len, out_actual);
}

/*
 * Common seekage code.
 * This is like Linux's fs/read_write.c:sys_llseek.
 */
static oskit_error_t
do_seek(gopenfile_t *ofile, oskit_s64_t ofs,
	oskit_seek_t whence, oskit_u64_t *out_newpos)
{
	struct file *file;
	struct task_struct ts;
	struct inode *inode;
	struct dentry *dentry;
	oskit_error_t err;
	int lerr;
	loff_t tmp;

	/* All params should've been verified in caller. */

	file = ofile->filp;
	dentry = file->f_dentry;
	inode  = dentry->d_inode;

	if (inode == NULL)
		return OSKIT_EBADF;

	/*
	 * If there is a fs-specific handler then call it but only for
	 * the signed long subset of long long.
	 */
	if (file->f_op && file->f_op->llseek) {
		if (ofs != (long)ofs)
			return OSKIT_E_INVALIDARG;
		err = fs_linux_create_current(&ts);
		if (err)
			return err;
		tmp = ofs;
		lerr = file->f_op->llseek(file, tmp, (int)whence);
		if (lerr >= 0)
			*out_newpos = lerr;
		fs_linux_destroy_current();
		return (lerr < 0) ? errno_to_oskit_error(-lerr) : 0;
	}

	tmp = -1;
	switch (whence) {
	case OSKIT_SEEK_SET:
		tmp = ofs;
		break;
	case OSKIT_SEEK_CUR:
		tmp = file->f_pos + ofs;
		break;
	case OSKIT_SEEK_END:
		if (inode == NULL)
			return OSKIT_E_INVALIDARG;
		tmp = inode->i_size + ofs;
		break;
	default:
		return OSKIT_E_INVALIDARG;
	}
	if (tmp < 0)
		return OSKIT_E_INVALIDARG;
	if (tmp != file->f_pos) {
		file->f_pos = tmp;
		file->f_reada = 0;
		file->f_version = ++event;
	}
	*out_newpos = file->f_pos;
	return 0;
}

/*
 * Just check params and call do_seek.
 */
static OSKIT_COMDECL
ofile_seek(oskit_openfile_t *f, oskit_s64_t ofs,
	   oskit_seek_t whence, oskit_u64_t *out_newpos)
{
	gopenfile_t *ofile;

	ofile = (gopenfile_t *)f;
	if (!verify_openfile(ofile)) return OSKIT_E_INVALIDARG;

	if (out_newpos == NULL)
		return OSKIT_E_INVALIDARG;

	return do_seek(ofile, ofs, whence, out_newpos);
}

/*
 * This just calls oskit_file_setstat on our underlying gfile_t.
 */
static OSKIT_COMDECL
ofile_setsize(oskit_openfile_t *f, oskit_u64_t new_size)
{
	gopenfile_t *ofile;
	oskit_stat_t stats;
	oskit_u32_t mask;

	ofile = (gopenfile_t *)f;
	if (!verify_openfile(ofile)) return OSKIT_E_INVALIDARG;

	mask = OSKIT_STAT_SIZE;
	stats.size = new_size;
	return oskit_file_setstat(&ofile->gf->filei, mask, &stats);
}

static OSKIT_COMDECL
no_ofile_copy_to(oskit_openfile_t *f,
	      oskit_stream_t *dst,
	      oskit_u64_t size,
	      oskit_u64_t *out_read,
	      oskit_u64_t *out_written)
{
	return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
no_ofile_commit(oskit_openfile_t *f, oskit_u32_t commit_flags)
{
	return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
no_ofile_revert(oskit_openfile_t *f)
{
	return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
no_ofile_lock_region(oskit_openfile_t *f,
		  oskit_u64_t offset, oskit_u64_t size,
		  oskit_u32_t lock_type)
{
	return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
no_ofile_unlock_region(oskit_openfile_t *f,
		    oskit_u64_t offset, oskit_u64_t size,
		    oskit_u32_t lock_type)
{
	return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
no_ofile_stat(oskit_openfile_t *f, oskit_stream_stat_t *out_stat,
	   oskit_u32_t stat_flags)
{
	return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
no_ofile_clone(oskit_openfile_t *f, oskit_openfile_t **out_stream)
{
	return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
ofile_getfile(oskit_openfile_t *f,
	      struct oskit_file **out_file)
{
	gopenfile_t *ofile;

	ofile = (gopenfile_t *)f;
	if (!verify_openfile(ofile)) return OSKIT_E_INVALIDARG;

	oskit_file_addref(&ofile->gf->filei);
	*out_file = &ofile->gf->filei;

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
	no_ofile_copy_to,
	no_ofile_commit,
	no_ofile_revert,
	no_ofile_lock_region,
	no_ofile_unlock_region,
	no_ofile_stat,			/* XXX should be doable */
	no_ofile_clone,			/* XXX should be doable */
	ofile_getfile,
};


/*
 * oskit_absio_t
 */

#define absio2gopenfile(a) ((gopenfile_t *)((char *)(a) \
					    - offsetof(gopenfile_t, absioi)))

static OSKIT_COMDECL
afile_query(oskit_absio_t *io,
	    const struct oskit_guid *iid,
	    void **out_ihandle)
{
	gopenfile_t *ofile;

	if (io == NULL || io->ops != &ofile_absio_ops)
		return OSKIT_E_INVALIDARG;

	ofile = absio2gopenfile(io);
	/* Checks on `ofile' done in oskit_openfile_query. */

	return oskit_openfile_query(&ofile->ofilei,iid,out_ihandle);
}

static OSKIT_COMDECL_U
afile_addref(oskit_absio_t *io)
{
	gopenfile_t *ofile;

	if (io == NULL || io->ops != &ofile_absio_ops)
		return OSKIT_E_INVALIDARG;

	ofile = absio2gopenfile(io);
	/* Checks on `ofile' done in oskit_openfile_addref. */

	return oskit_openfile_addref(&ofile->ofilei);
}

static OSKIT_COMDECL_U
afile_release(oskit_absio_t *io)
{
	gopenfile_t *ofile;

	if (io == NULL || io->ops != &ofile_absio_ops)
		return OSKIT_E_INVALIDARG;
    
	ofile = absio2gopenfile(io);
	/* Checks on `ofile' done in oskit_openfile_release. */

	return oskit_openfile_release(&ofile->ofilei);
}

static OSKIT_COMDECL
afile_read(oskit_absio_t *io, void *buf,
	   oskit_off_t offset, oskit_size_t amount,
	   oskit_size_t *out_actual)
{
	gopenfile_t *ofile;
	oskit_u64_t newpos;
	oskit_error_t err;

	if (io == NULL || io->ops != &ofile_absio_ops)
		return OSKIT_E_INVALIDARG;
    
	ofile = absio2gopenfile(io);
	if (!verify_openfile(ofile)) return OSKIT_E_INVALIDARG;
	
	if (buf == NULL || out_actual == NULL)
		return OSKIT_E_INVALIDARG;

	/* Maybe nothing to do. */
	if (amount == 0) {
		*out_actual = 0;
		return 0;
	}

	/* XXX do we need to restore this seek pointer later? */
	err = do_seek(ofile, offset, OSKIT_SEEK_SET, &newpos);
	if (err)
		return err;

	return do_rw(FMODE_READ, ofile, buf, amount, out_actual);
}

static OSKIT_COMDECL
afile_write(oskit_absio_t *io, const void *buf,
	    oskit_off_t offset, oskit_size_t amount,
	    oskit_size_t *out_actual)
{
	gopenfile_t *ofile;
	oskit_u64_t newpos;
	oskit_error_t err;

	if (io == NULL || io->ops != &ofile_absio_ops)
		return OSKIT_E_INVALIDARG;
    
	ofile = absio2gopenfile(io);
	if (!verify_openfile(ofile)) return OSKIT_E_INVALIDARG;
	
	if (buf == NULL || out_actual == NULL)
		return OSKIT_E_INVALIDARG;

	/* Maybe nothing to do. */
	if (amount == 0) {
		*out_actual = 0;
		return 0;
	}

	/* XXX do we need to restore this seek pointer later? */
	err = do_seek(ofile, offset, OSKIT_SEEK_SET, &newpos);
	if (err)
		return err;

	return do_rw(FMODE_WRITE, ofile, (void *)buf, amount, out_actual);
}

static OSKIT_COMDECL
afile_getsize(oskit_absio_t *io, oskit_off_t *out_size)
{
	gopenfile_t *ofile;
	oskit_stat_t sb;
	oskit_error_t err;

	if (io == NULL || io->ops != &ofile_absio_ops)
		return OSKIT_E_INVALIDARG;
    
	ofile = absio2gopenfile(io);
	/* Checks on `ofile' done in stat. */

	err = oskit_file_stat(&ofile->gf->filei, &sb);
	if (err)
		return err;

	*out_size = sb.size;
	return 0;
}

static OSKIT_COMDECL
afile_setsize(oskit_absio_t *io, oskit_off_t new_size)
{
	gopenfile_t *ofile;
	oskit_stat_t sb;

	if (io == NULL || io->ops != &ofile_absio_ops)
		return OSKIT_E_INVALIDARG;
    
	ofile = absio2gopenfile(io);
	/* Checks on `ofile' done in setstat. */

	sb.size = new_size;
	return oskit_file_setstat(&ofile->gf->filei, OSKIT_STAT_SIZE, &sb);
}

static struct oskit_absio_ops ofile_absio_ops = {
	afile_query,
	afile_addref,
	afile_release,
	0,				/* slot reserved for getblocksize */
	afile_read,
	afile_write,
	afile_getsize,
	afile_setsize,
};


/*
 * Create and initialize a gopenfile.
 * oskit_openfile_release is the opposite of this.
 * NOTE: the struct file passed to this must have beem kmalloc'd (see
 * oskit_openfile_release).
 */
extern oskit_error_t
gopenfile_create(gfile_t *file, struct file *f,
		 gopenfile_t **out_openfile)
{
	gopenfile_t *ofile;

	if (out_openfile == NULL)
		return OSKIT_E_INVALIDARG;

	ofile = kmalloc(sizeof(gopenfile_t), GFP_KERNEL);
	if (ofile == NULL)
		return OSKIT_ENOMEM;
	ofile->ofilei.ops = &ofile_ops;
	ofile->absioi.ops = &ofile_absio_ops;
	ofile->count = 1;
	ofile->gf = file;
	oskit_file_addref(&file->filei);
	ofile->filp = f;
	f->f_count++;

	*out_openfile = ofile;
	return 0;
}

