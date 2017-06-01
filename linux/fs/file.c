/*
 * Copyright (c) 1997-2000 The University of Utah and the Flux Group.
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
 * Implements the oskit_file_t, oskit_absio_t, and oskit_dir_t interfaces.
 *
 * oskit_file and oskit_dir also implement absio interfaces,
 * and share code.
 */

#include <oskit/fs/file.h>
#include <oskit/io/absio.h>
#include <oskit/fs/dir.h>
#include <oskit/c/stdlib.h>		/* for malloc */
#include <oskit/c/alloca.h>

#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/stat.h>
#include <linux/malloc.h>
#include <linux/stat.h>
#include <linux/file.h>

#include "current.h"
#include "errno.h"
#include "types.h"

#include <assert.h>
#include "hashtab.h"
extern hashtab_t	dentrytab;	/* dentry -> oskit_file_t */

/* So we don't need <oskit/c/stddef.h>. */
#define	offsetof(type, member)	((size_t)(&((type *)0)->member))

static struct oskit_file_ops file_ops;
static struct oskit_absio_ops file_absio_ops;
static struct oskit_dir_ops dir_ops;

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
 * oskit_file_t
 */

static OSKIT_COMDECL
file_query(oskit_file_t *f,
	   const struct oskit_guid *iid,
	   void **out_ihandle)
{
	gfile_t *file;

	file = (gfile_t *)f;
	if (!verify_file(file)) return OSKIT_E_INVALIDARG;

	if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
	    memcmp(iid, &oskit_posixio_iid, sizeof(*iid)) == 0 ||
	    memcmp(iid, &oskit_file_iid, sizeof(*iid)) == 0) {
		*out_ihandle = &file->filei;
		++file->count;
		return 0;
	}

	if (memcmp(iid, &oskit_absio_iid, sizeof(*iid)) == 0) {
		*out_ihandle = &file->absioi;
		++file->count;
		return 0;
	}

	*out_ihandle = NULL;
	return OSKIT_E_NOINTERFACE;
}

static OSKIT_COMDECL_U
file_addref(oskit_file_t *f)
{
	gfile_t *file;

	file = (gfile_t *)f;
	VERIFY_OR_PANIC(file, "file");

	return ++file->count;
}

/*
 * This needs to undo what gfile_create does.
 * See also dir_release.
 */
static OSKIT_COMDECL_U
file_release(oskit_file_t *f)
{
	gfile_t *file;
	unsigned newcount;

	file = (gfile_t *)f;
	VERIFY_OR_PANIC(file, "file");

	newcount = --file->count;
	if (newcount == 0) {
		int rc;

		oskit_filesystem_release(&file->fs->fsi);

		rc = hashtab_remove(dentrytab,
				    (hashtab_key_t) file->dentry, 0, 0);
		assert(rc == HASHTAB_SUCCESS);

		dput(file->dentry);
		kfree(file);
	}

	return newcount;
}

/*
 * This is almost identical to Linux's fs/stat.c:cp_new_stat.
 * It just reads the values out of the inode struct.
 */
static OSKIT_COMDECL
file_stat(oskit_file_t *f,
	  struct oskit_stat *out_stats)
{
	gfile_t *file;
	struct inode *inode;
	struct dentry *dentry;
	unsigned int blocks, indirect;

	file = (gfile_t *)f;
	if (!verify_file(file)) return OSKIT_E_INVALIDARG;
	dentry = file->dentry;
	inode = dentry->d_inode;

	if (out_stats == NULL)
		return OSKIT_E_INVALIDARG;

	memset(out_stats, 0, sizeof(*out_stats));
	out_stats->dev		 = kdev_t_to_nr(inode->i_dev);
	out_stats->ino		 = inode->i_ino;
	out_stats->mode		 = inode->i_mode;
	out_stats->nlink	 = inode->i_nlink;
	out_stats->uid		 = inode->i_uid;
	out_stats->gid		 = inode->i_gid;
	out_stats->rdev		 = kdev_t_to_nr(inode->i_rdev);
	out_stats->size		 = inode->i_size;
	out_stats->atime.tv_sec	 = inode->i_atime;
	out_stats->mtime.tv_sec	 = inode->i_mtime;
	out_stats->ctime.tv_sec	 = inode->i_ctime;

	/*
	 * st_blocks and st_blksize are approximated with a simple algorithm if
	 * they aren't supported directly by the filesystem. The minix and msdos
	 * filesystems don't keep track of blocks, so they would either have to
	 * be counted explicitly (by delving into the file itself), or by using
	 * this simple algorithm to get a reasonable (although not 100% accurate)
	 * value.
	 */

	/*
	 * Use minix fs values for the number of direct and indirect blocks.  The
	 * count is now exact for the minix fs except that it counts zero blocks.
	 * Everything is in BLOCK_SIZE'd units until the assignment to
	 * out_stats->blksize.
	 */
#define D_B   7
#define I_B   (BLOCK_SIZE / sizeof(unsigned short))

	if (!inode->i_blksize) {
		blocks = (out_stats->size + BLOCK_SIZE - 1) >> BLOCK_SIZE_BITS;
		if (blocks > D_B) {
			indirect = (blocks - D_B + I_B - 1) / I_B;
			blocks += indirect;
			if (indirect > 1) {
				indirect = (indirect - 1 + I_B - 1) / I_B;
				blocks += indirect;
				if (indirect > 1)
					blocks++;
			}
		}
		out_stats->blocks = (BLOCK_SIZE / 512) * blocks;
		out_stats->blksize = BLOCK_SIZE;
	} else {
		out_stats->blocks = inode->i_blocks;
		out_stats->blksize = inode->i_blksize;
	}

	return 0;
}

/*
 * This fills in an iattr struct and calls notify_change.
 * This is Linux's sys_chmod, sys_chown, etc all rolled into one.
 */
static OSKIT_COMDECL
file_setstat(oskit_file_t *f, oskit_u32_t mask,
	     const struct oskit_stat *stats)
{
	gfile_t *file;
	struct task_struct ts;
	struct iattr attr;
	struct dentry *dentry;
	struct inode *inode;
	int lerr;
	oskit_error_t err;

	file = (gfile_t *)f;
	if (!verify_file(file)) return OSKIT_E_INVALIDARG;
	dentry = file->dentry;
	inode = dentry->d_inode;

	if (stats == NULL)
		return OSKIT_E_INVALIDARG;

	/*
	 * General checks.
	 * It is interesting to note that Linux has a little bug where
	 * you can change the time on an append-only file,
	 * however we don't have that problem.
	 */

	if (mask & ~OSKIT_STAT_SETTABLE)
		return OSKIT_E_INVALIDARG;
	/* Can't change attrs of symlinks. */
	if (S_ISLNK(inode->i_mode))
		return OSKIT_E_NOTIMPL;
	/* Check settings on filesystem (IS_RDONLY) and inode. */
	if (IS_RDONLY(inode))
		return OSKIT_EROFS;
	if (IS_IMMUTABLE(inode) || IS_APPEND(inode))
		return OSKIT_EPERM;

	/*
	 * Create `current'; be sure to destroy it before leaving.
	 */
	err = fs_linux_create_current(&ts);
	if (err)
		return err;

	/*
	 * Set up `attr' according to mask.
	 */
	attr.ia_valid = 0;
	if (mask & OSKIT_STAT_MODE) {
		attr.ia_mode = (stats->mode & S_IALLUGO) | (inode->i_mode & ~S_IALLUGO);
		attr.ia_valid |= ATTR_MODE;
	}
	if (mask & OSKIT_STAT_UID) {
		attr.ia_uid = stats->uid;
		attr.ia_valid |= ATTR_UID;

		/* Clear the setuid bit if set. */
		if (inode->i_mode & S_ISUID) {
			attr.ia_mode = (inode->i_mode & ~S_ISUID);
			attr.ia_valid |= ATTR_MODE;
		}
	}
	if (mask & OSKIT_STAT_GID) {
		attr.ia_gid = stats->gid;
		attr.ia_valid |= ATTR_GID;

		/*
		 * Clear the setgid bit if set but only if group execute is
		 * there too.
		 * Otherwise the setgid bit means "mandatory locking",
		 * which isn't implemented in the OSKit but hey.
		 */
		if (((inode->i_mode & (S_ISGID | S_IXGRP)) == (S_ISGID | S_IXGRP))) {
			attr.ia_mode = (inode->i_mode & ~S_ISGID);
			attr.ia_valid |= ATTR_MODE;
		}
	}
	if (mask & OSKIT_STAT_ATIME) {
		attr.ia_atime = stats->atime.tv_sec;
		attr.ia_valid |= (ATTR_ATIME | ATTR_ATIME_SET);
	}
	if (mask & OSKIT_STAT_MTIME) {
		attr.ia_mtime = stats->mtime.tv_sec;
		attr.ia_valid |= (ATTR_MTIME | ATTR_MTIME_SET);
	}
	if (mask & OSKIT_STAT_UTIMES_NULL) {
		attr.ia_valid |= (ATTR_ATIME | ATTR_MTIME);
	}
	if (mask & OSKIT_STAT_SIZE) {
		lerr = -EISDIR;
		if (S_ISDIR(inode->i_mode))
			goto done;
		lerr = permission(inode, MAY_WRITE);
		if (lerr)
			goto done;
		lerr = get_write_access(inode);
		if (lerr)
			goto done;
		lerr = do_truncate(dentry, (unsigned long)stats->size);
		if (lerr)
			goto done;
	}

	attr.ia_valid |= ATTR_CTIME;
	/* notify_change marks the inode dirty */
	lerr = notify_change(dentry, &attr);
 done:
	fs_linux_destroy_current();
	return errno_to_oskit_error(-lerr);
}

/*
 * Linux doesn't have any sort of pathconf facility.
 */
static OSKIT_COMDECL
file_pathconf(oskit_file_t *f, oskit_s32_t option,
	      oskit_s32_t *out_val)
{
	return OSKIT_E_NOTIMPL;
}

/*
 * File sync/datasync.  This always waits.
 *
 * This is similar to Linux's fs/buffer.c:sys_fsync.
 * It just calls the `fsync' file_operation.
 * However all we have is a struct inode,
 * but we know that i_op->default_file_ops has a pointer
 * to the correct fsync file_operation.
 */
static OSKIT_COMDECL
file_sync(oskit_file_t *f, oskit_bool_t ignored_wait)
{
	gfile_t *file;
	struct file_operations *fops;
	struct task_struct ts;
	oskit_error_t err;
	struct dentry *dentry;
	struct inode *inode;

	file = (gfile_t *)f;
	if (!verify_file(file)) return OSKIT_E_INVALIDARG;
	dentry = file->dentry;
	inode  = dentry->d_inode;

	if (inode->i_op == NULL ||
	    inode->i_op->default_file_ops == NULL ||
	    inode->i_op->default_file_ops->fsync == NULL)
		return OSKIT_E_INVALIDARG;
	fops = inode->i_op->default_file_ops;

	err = fs_linux_create_current(&ts);
	if (err)
		return err;

	/*
	 * We pass a null `struct file *' here becuase none of the
	 * filesystems use it.
	 * And besides, we don't have one!
	 */
	if (fops->fsync(NULL, dentry) != 0) {
		fs_linux_destroy_current();
		return OSKIT_EIO;
	}

	fs_linux_destroy_current();
	return 0;
}

/*
 * This is like Linux's fs/open.c:sys_access.
 * It just ends up calling `permission'.
 */
static OSKIT_COMDECL
file_access(oskit_file_t *f, oskit_amode_t mask)
{
	gfile_t *file;
	int old_fsuid, old_fsgid;
	struct task_struct ts;
	struct dentry *dentry;
	struct inode *inode;
	oskit_error_t err;
	int res;
	int mode;

	file = (gfile_t *)f;
	if (!verify_file(file)) return OSKIT_E_INVALIDARG;
	dentry = file->dentry;
	inode  = dentry->d_inode;

	err = fs_linux_create_current(&ts);
	if (err)
		return err;

	mode = 0;
	if (mask & OSKIT_R_OK) mode |= MAY_READ;
	if (mask & OSKIT_W_OK) mode |= MAY_WRITE;
	if (mask & OSKIT_X_OK) mode |= MAY_EXEC;

	old_fsuid = current->fsuid;
	old_fsgid = current->fsgid;
	current->fsuid = current->uid;
	current->fsgid = current->gid;
	res = permission(inode, mode);
	current->fsuid = old_fsuid;
	current->fsgid = old_fsgid;

	fs_linux_destroy_current();
	return errno_to_oskit_error(-res);
}

/*
 * This is like Linux's fs/stat.c:sys_readlink.
 * Note that it doesn't put a NUL on the end.
 */
static OSKIT_COMDECL
file_readlink(oskit_file_t *f,
	      char *buf, oskit_u32_t len,
	      oskit_u32_t *out_actual)
{
	gfile_t *file;
	struct task_struct ts;
	oskit_error_t err;
	struct inode *inode;
	struct dentry *dentry;

	file = (gfile_t *)f;
	if (!verify_file(file)) return OSKIT_E_INVALIDARG;
	dentry = file->dentry;
	inode = dentry->d_inode;

	if (len <= 0 || buf == NULL ||
	    inode->i_op == NULL || inode->i_op->readlink == NULL ||
	    out_actual == NULL)
		return OSKIT_E_INVALIDARG;

	err = fs_linux_create_current(&ts);
	if (err)
		return err;

	*out_actual = inode->i_op->readlink(dentry, buf, len);
	if (*out_actual == 0) {
		fs_linux_destroy_current();
		return OSKIT_E_FAIL;
	}

	fs_linux_destroy_current();
	return 0;
}

/*
 * Create an instance of oskit_openfile_t from this oskit_file_t.
 *
 * This boils down to something like Linux's fs/open.c:do_open.
 */
static OSKIT_COMDECL
file_open(oskit_file_t *f, oskit_oflags_t iflags,
	  struct oskit_openfile **out_openfile)
{
	gfile_t *file;
	gopenfile_t *ofile;
	struct file *filp;
	struct task_struct ts;
	struct dentry *dentry;
	oskit_error_t err;
	int flags;
	int lerr;
	int perms;

	/*
	 * Checks on params.
	 */
	file = (gfile_t *)f;
	if (!verify_file(file)) return OSKIT_E_INVALIDARG;
	dentry = file->dentry;
	if (out_openfile == NULL)
		return OSKIT_E_INVALIDARG;
	/* Can only open dirs and regular files. */
	if (! S_ISDIR(dentry->d_inode->i_mode) &&
	    ! S_ISREG(dentry->d_inode->i_mode))
		return OSKIT_E_INVALIDARG;

	/*
	 * Convert iflags into Linux form.
	 * Also figure out what perms are needed.
	 *
	 * For some retarded reason (O_RDONLY | O_WRONLY) != O_RDWR in Linux.
	 */
	flags = 0;
	perms = 0;
	if (iflags & ~(OSKIT_O_RDONLY |
		       OSKIT_O_WRONLY |
		       OSKIT_O_RDWR |
		       OSKIT_O_TRUNC |
		       OSKIT_O_APPEND |
		       OSKIT_O_DSYNC |
		       OSKIT_O_SYNC))
		return OSKIT_E_INVALIDARG;
	if (iflags & OSKIT_O_RDONLY) {
		flags |= O_RDONLY;
		perms |= MAY_READ;
	}
	if (iflags & OSKIT_O_WRONLY) {
		flags |= O_WRONLY;
		perms |= MAY_WRITE;
	}
	if ((iflags & OSKIT_O_RDWR) == OSKIT_O_RDWR) {
		flags &= ~(O_WRONLY | O_RDONLY);
		flags |= O_RDWR;
		perms |= (MAY_READ | MAY_WRITE);
	}
	if (iflags & OSKIT_O_TRUNC) {
		flags |= O_TRUNC;
		perms |= MAY_WRITE;
	}
	if (iflags & OSKIT_O_APPEND)
		flags |= O_APPEND;
	if (iflags & OSKIT_O_DSYNC)
		flags |= O_SYNC;
	if (iflags & OSKIT_O_SYNC)
		flags |= O_SYNC;

/**/	err = fs_linux_create_current(&ts);
	if (err)
		return err;

	/*
	 * Permission checks.
	 */
	/* Directories can't be written. */
	if ((perms & MAY_WRITE) && S_ISDIR(dentry->d_inode->i_mode)) {
		err = OSKIT_EISDIR;
		goto cleanup_current;
	}
	/* Check file for readability/writability. */
	lerr = permission(dentry->d_inode, perms);
	if (lerr) {
		err = errno_to_oskit_error(-lerr);
		goto cleanup_current;
	}
	/* Check filesystem for readonlyness. */
	if ((perms & MAY_WRITE) && IS_RDONLY(dentry->d_inode)) {
		err = errno_to_oskit_error(EROFS);
		goto cleanup_current;
	}
	/* An append-only file must be opened in append mode for writing. */
	if ((perms & MAY_WRITE) &&
	    IS_APPEND(dentry->d_inode) && !(flags & O_APPEND)) {
		err = OSKIT_EPERM;
		goto cleanup_current;
	}

	/*
	 * Get a struct file and fill it in.
	 * Then call the open f_op if there is one.
	 * This is like fs/file_table.c:get_empty_filp and fs/open.c:do_open
	 * combined.
	 */
/**/	filp = kmalloc(sizeof(struct file), GFP_KERNEL);
	if (filp == NULL) {
		err = OSKIT_ENOMEM;
		goto cleanup_current;
	}
	memset(filp, 0, sizeof(*filp));
	filp->f_count = 1;
	filp->f_version = ++event;
	filp->f_flags = flags;
	filp->f_mode = (filp->f_flags + 1) & O_ACCMODE;
	filp->f_dentry = dentry;
	dget(dentry);
	filp->f_pos = 0;
	filp->f_reada = 0;
	filp->f_op = NULL;
	if (dentry->d_inode->i_op)
		filp->f_op = dentry->d_inode->i_op->default_file_ops;
	if (filp->f_op && filp->f_op->open) {
		lerr = filp->f_op->open(dentry->d_inode, filp);
		if (lerr) {
			err = errno_to_oskit_error(-lerr);
			goto cleanup_inode_filp_current;
		}
	}
	filp->f_flags &= ~(O_CREAT | O_EXCL | O_NOCTTY | O_TRUNC);

	/*
	 * Truncate if they want it.
	 */
	if (flags & O_TRUNC) {
		lerr = do_truncate(dentry, 0);
		if (lerr) {
			err = errno_to_oskit_error(-lerr);
			goto cleanup_inode_filp_current;
		}
	}

	/*
	 * Get a oskit_openfile_t to return.
	 */
	err = gopenfile_create(file, filp, &ofile);
	if (err)
		goto cleanup_inode_filp_current;
	fput(filp);
	*out_openfile = &ofile->ofilei;
	goto cleanup_current;

 cleanup_inode_filp_current:
	dput(dentry);
	kfree(filp);
 cleanup_current:
	fs_linux_destroy_current();
	return err;
}

static OSKIT_COMDECL
file_getfs(oskit_file_t *f,
	   struct oskit_filesystem **out_fs)
{
	gfile_t *file;

	file = (gfile_t *)f;
	if (!verify_file(file)) return OSKIT_E_INVALIDARG;

	*out_fs = &file->fs->fsi;
	oskit_filesystem_addref(&file->fs->fsi);

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
 * oskit_absio_t implemented on oskit_file_t's
 */

#define absio2gfile(a) ((gfile_t *)((char *)(a) - offsetof(gfile_t, absioi)))

static OSKIT_COMDECL
afile_query(oskit_absio_t *io,
	    const struct oskit_guid *iid,
	    void **out_ihandle)
{
	gfile_t *file;

	if (io == NULL || io->ops != &file_absio_ops)
		return OSKIT_E_INVALIDARG;

	file = absio2gfile(io);
	/* Checks on `file' done in oskit_file_query. */

	/* May be file_query or dir_query. */
	return oskit_file_query(&file->filei, iid, out_ihandle);
}

static OSKIT_COMDECL_U
afile_addref(oskit_absio_t *io)
{
	gfile_t *file;

	if (io == NULL || io->ops != &file_absio_ops)
		return OSKIT_E_INVALIDARG;

	file = absio2gfile(io);
	/* Checks on `file' done in file_addref. */

	return file_addref(&file->filei);
}

static OSKIT_COMDECL_U
afile_release(oskit_absio_t *io)
{
	gfile_t *file;

	if (io == NULL || io->ops != &file_absio_ops)
		return OSKIT_E_INVALIDARG;

	file = absio2gfile(io);
	/* Checks on `file' done in oskit_file_release. */

	/* May be file_release or dir_release. */
	return oskit_file_release(&file->filei);
}

/*
 * This is mega-lame but is easier then reimplementing portions of file_open
 * and openfile_read.
 */
static OSKIT_COMDECL
afile_read(oskit_absio_t *io, void *buf,
	   oskit_off_t offset, oskit_size_t amount,
	   oskit_size_t *out_actual)
{
	gfile_t *file;
	oskit_openfile_t *ofile;
	oskit_error_t err;
	oskit_off_t newoffset;

	if (io == NULL || io->ops != &file_absio_ops)
		return OSKIT_E_INVALIDARG;

	file = absio2gfile(io);
	/* Checks on `file' done in open. */

	err = oskit_file_open(&file->filei, OSKIT_O_RDONLY, &ofile);
	if (err)
		return err;

	err = oskit_openfile_seek(ofile, offset, OSKIT_SEEK_SET, &newoffset);
	if (err) {
		oskit_openfile_release(ofile);
		return err;
	}

	err = oskit_openfile_read(ofile, buf, amount, out_actual);
	oskit_openfile_release(ofile);
	return err;
}

/*
 * This too is mega-lame but is easier then reimplementing portions of file_open
 * and openfile_write.
 */
static OSKIT_COMDECL
afile_write(oskit_absio_t *io, const void *buf,
	    oskit_off_t offset, oskit_size_t amount,
	    oskit_size_t *out_actual)
{
	gfile_t *file;
	oskit_openfile_t *ofile;
	oskit_error_t err;
	oskit_off_t newoffset;

	if (io == NULL || io->ops != &file_absio_ops)
		return OSKIT_E_INVALIDARG;

	file = absio2gfile(io);
	/* Checks on `file' done in open. */

	err = oskit_file_open(&file->filei, OSKIT_O_WRONLY, &ofile);
	if (err)
		return err;

	err = oskit_openfile_seek(ofile, offset, OSKIT_SEEK_SET, &newoffset);
	if (err) {
		oskit_openfile_release(ofile);
		return err;
	}

	err = oskit_openfile_write(ofile, buf, amount, out_actual);
	oskit_openfile_release(ofile);
	return err;
}

static OSKIT_COMDECL
afile_getsize(oskit_absio_t *io, oskit_off_t *out_size)
{
	oskit_stat_t sb;
	gfile_t *file;
	oskit_error_t err;

	if (io == NULL || io->ops != &file_absio_ops)
		return OSKIT_E_INVALIDARG;

	file = absio2gfile(io);
	/* Checks on `file' done in stat. */

	err = oskit_file_stat(&file->filei, &sb);
	if (err)
		return err;

	*out_size = sb.size;
	return 0;
}

static OSKIT_COMDECL
afile_setsize(oskit_absio_t *io, oskit_off_t new_size)
{
	oskit_stat_t sb;
	gfile_t *file;

	if (io == NULL || io->ops != &file_absio_ops)
		return OSKIT_E_INVALIDARG;

	file = absio2gfile(io);
	/* Checks on `file' done in setstat. */

	sb.size = new_size;
	return oskit_file_setstat(&file->filei, OSKIT_STAT_SIZE, &sb);
}

static struct oskit_absio_ops file_absio_ops = {
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
 * oskit_dir_t
 */

static OSKIT_COMDECL
dir_query(oskit_dir_t *d,
	  const struct oskit_guid *iid,
	  void **out_ihandle)
{
	gdir_t *dir;

	dir = (gdir_t *)d;
	if (!verify_dir(dir)) return OSKIT_E_INVALIDARG;

	if (memcmp(iid, &oskit_dir_iid, sizeof(*iid)) == 0) {
		*out_ihandle = &dir->diri;
		++dir->count;
		return 0;
	}

	return file_query((oskit_file_t *)d, iid, out_ihandle);
}

/*
 * This needs to undo what gfile_create does.
 * See also file_release.
 */
static OSKIT_COMDECL_U
dir_release(oskit_dir_t *d)
{
	gdir_t *dir;
	unsigned newcount;

	dir = (gdir_t *)d;
	VERIFY_OR_PANIC(dir, "directory");

	newcount = --dir->count;
	if (newcount == 0) {
		int rc;

		/* This is the difference between this method and file_release. */
		if (dir->vparent && dir->vparent != d)
			oskit_dir_release(dir->vparent);

		oskit_filesystem_release(&dir->fs->fsi);

		rc = hashtab_remove(dentrytab,
				    (hashtab_key_t) dir->dentry, 0, 0);
		assert(rc == HASHTAB_SUCCESS);

		dput(dir->dentry);
		kfree(dir);
	}

	return newcount;
}

/*
 * This just ends up calling Linux's `lookup',
 * unless looking up `..' in a reparented dir.
 */
static OSKIT_COMDECL
dir_lookup(oskit_dir_t *d, const char *name,
	   oskit_file_t **out_file)
{
	gdir_t *dir;
	gfile_t *file;
	struct task_struct ts;
	oskit_error_t err;
	struct dentry *dentry;
	int lerr;

	dir = (gdir_t *)d;
	if (!verify_dir(dir)) return OSKIT_E_INVALIDARG;

	if (name == NULL || out_file == NULL)
		return OSKIT_E_INVALIDARG;

	/*
	 * A lookup of "" refers to the directory itself.
	 */
	if (name[0] == '\0') {
		*out_file = (oskit_file_t *)d;
		oskit_dir_addref(d);
		return 0;
	}

	/* No more than one component at a time. */
	if (strchr (name, '/') != NULL)
		return OSKIT_E_INVALIDARG;

	err = fs_linux_create_current(&ts);
	if (err)
		return err;

	/*
	 * If looking up `..' and have been reparented,
	 * give them our parent,
	 * if they have access to search us.
	 */
	if (dir->vparent && strcmp(name, "..") == 0) {
		lerr = permission(dir->dentry->d_inode, MAY_EXEC);
		if (lerr) {
			fs_linux_destroy_current();
			return errno_to_oskit_error(-lerr);
		}

		*out_file = (oskit_file_t *)dir->vparent;
		oskit_dir_addref(dir->vparent);

		fs_linux_destroy_current();
		return 0;
	}

	/*
	 * Call Linux's `lookup'.
	 * We have to add a reference to the current directory since lookup
	 * does a release on it.
	 */
	dget(dir->dentry);
	dentry = lookup_dentry(name, dir->dentry, 0);
	if (IS_ERR(dentry)) {
		fs_linux_destroy_current();
		return errno_to_oskit_error(-(PTR_ERR(dentry)));
	}

	/*
	 * So, lookup_dentry can return a dentry, but with no inode if
	 * the file does not exist.
	 */
	if (!dentry->d_inode) {
		dput(dentry);
		fs_linux_destroy_current();
		return errno_to_oskit_error(ENOENT);
	}

	err = gfile_lookup(dir->fs, dentry, &file);
	if (err) {
		dput(dentry);
		fs_linux_destroy_current();
		return err;
	}

	dput(dentry);
	fs_linux_destroy_current();
	*out_file = &file->filei;
	return 0;
}

/*
 * This just calls Linux's fs/namei.c:open_namei with O_CREAT and
 * possibly O_EXCL.
 */
static OSKIT_COMDECL
dir_create(oskit_dir_t *d, const char *name,
	   oskit_bool_t excl, oskit_mode_t mode,
	   oskit_file_t **out_file)
{
	gdir_t *dir;
	gfile_t *file;
	struct task_struct ts;
	oskit_error_t err;
	int flags;
	struct dentry *dentry;

	dir = (gdir_t *)d;
	if (!verify_dir(dir)) return OSKIT_E_INVALIDARG;

	if (name == NULL || out_file == NULL)
		return OSKIT_E_INVALIDARG;

	/* No more than one component at a time. */
	if (strchr (name, '/') != NULL)
		return OSKIT_E_INVALIDARG;

	err = fs_linux_create_current(&ts);
	if (err)
		return err;

	/*
	 * Note that the flag munging done in Linux's do_open doesn't
	 * apply to us because we are only creating.
	 */
	flags = O_CREAT;
	if (excl)
		flags |= O_EXCL;

	current->fs->pwd = dir->dentry;
	dget(dir->dentry);
	dentry = open_namei(name, flags, mode);
	current->fs->pwd = NULL;
	dput(dir->dentry);

	if (IS_ERR(dentry)) {
		fs_linux_destroy_current();
		return errno_to_oskit_error(-(PTR_ERR(dentry)));
	}

	err = gfile_lookup(dir->fs, dentry, &file);
	if (err) {
		dput(dentry);
		fs_linux_destroy_current();
		return err;
	}

	dput(dentry);
	fs_linux_destroy_current();
	*out_file = &file->filei;
	return 0;
}

/*
 * This is like Linux's fs/namei.c:sys_link.
 */
static OSKIT_COMDECL
dir_link(oskit_dir_t *d, const char *name,
	 oskit_file_t *f)
{
	gdir_t *dir;
	gfile_t *file;
	struct task_struct ts;
	oskit_error_t err;
	int lerr;

	dir = (gdir_t *)d;
	if (!verify_dir(dir)) return OSKIT_E_INVALIDARG;

	if (name == NULL || f == NULL)
		return OSKIT_E_INVALIDARG;

	/* No more than one component at a time. */
	if (strchr (name, '/') != NULL)
		return OSKIT_E_INVALIDARG;

	/* Make sure the `f' is one of ours, so we know it is really a gfile. */
	if (f->ops != &file_ops && f->ops != (struct oskit_file_ops *)&dir_ops)
		return OSKIT_EXDEV;
	file = (gfile_t *)f;
	if (!verify_file(file)) return OSKIT_E_INVALIDARG;

	err = fs_linux_create_current(&ts);
	if (err)
		return err;

	current->fs->pwd = dir->dentry;
	dget(dir->dentry);
	lerr = do_link(file->dentry, name);
	current->fs->pwd = NULL;
	dput(dir->dentry);

	fs_linux_destroy_current();

	return errno_to_oskit_error(-lerr);
}

/*
 * This is like Linux's fs/namei.c:sys_unlink.
 */
static OSKIT_COMDECL
dir_unlink(oskit_dir_t *d, const char *name)
{
	gdir_t *dir;
	struct task_struct ts;
	oskit_error_t err;
	int lerr;

	dir = (gdir_t *)d;
	if (!verify_dir(dir)) return OSKIT_E_INVALIDARG;

	/* No more than one component at a time. */
	if (name == NULL || strchr (name, '/') != NULL)
		return OSKIT_E_INVALIDARG;

	err = fs_linux_create_current(&ts);
	if (err)
		return err;

	current->fs->pwd = dir->dentry;
	dget(dir->dentry);
	lerr = do_unlink(name);
	current->fs->pwd = NULL;
	dput(dir->dentry);

	fs_linux_destroy_current();

	return errno_to_oskit_error(-lerr);
}

/*
 * This is like Linux's fs/namei.c:sys_rename.
 */
static OSKIT_COMDECL
dir_rename(oskit_dir_t *old_d, const char *old_name,
	   oskit_dir_t *new_d, const char *new_name)
{
	gdir_t *old_dir, *new_dir;
	char *old_path;
	struct task_struct ts;
	oskit_error_t err;
	int lerr;

	old_dir = (gdir_t *)old_d;
	if (!verify_dir(old_dir)) return OSKIT_E_INVALIDARG;
	new_dir = (gdir_t *)new_d;
	if (!verify_dir(new_dir)) return OSKIT_E_INVALIDARG;

	/* Make sure old_d and new_d are our kind. */
	if (old_d->ops != &dir_ops || new_d->ops != &dir_ops)
		return OSKIT_EXDEV;

	/* No more than one component at a time. */
	if (old_name == NULL || strchr (old_name, '/') != NULL ||
	    new_name == NULL || strchr(new_name, '/') != NULL)
		return OSKIT_E_INVALIDARG;

	/*
	 * do_rename assumes that both names are looked up relative to
	 * the same (root, pwd) pair.
	 * However, we need the lookup of old_name to be relative to
	 * old_dir and the lookup of new_name to be relative to
	 * new_dir.
	 * So, we use old_dir as root, new_dir as pwd,
	 * and prepend a `/' to old_name.
	 * Sigh.
	 */
	old_path = alloca(strlen(old_name)+2);
	if (old_path == NULL)
		return OSKIT_ENOMEM;
	sprintf(old_path, "/%s", old_name);

	err = fs_linux_create_current(&ts);
	if (err)
		return err;

	current->fs->root = old_dir->dentry; dget(old_dir->dentry);
	current->fs->pwd  = new_dir->dentry; dget(new_dir->dentry);
	lerr = do_rename(old_path, new_name);
	current->fs->root = NULL; dput(old_dir->dentry);
	current->fs->pwd = NULL;  dput(new_dir->dentry);

	fs_linux_destroy_current();
	return errno_to_oskit_error(-lerr);
}

/*
 * This is like Linux's fs/namei.c:sys_mkdir.
 */
static OSKIT_COMDECL
dir_mkdir(oskit_dir_t *d, const char *name,
	  oskit_mode_t mode)
{
	gdir_t *dir;
	struct task_struct ts;
	oskit_error_t err;
	int lerr;

	dir = (gdir_t *)d;
	if (!verify_dir(dir)) return OSKIT_E_INVALIDARG;

	/* No more than one component at a time. */
	if (name == NULL || strchr (name, '/') != NULL)
		return OSKIT_E_INVALIDARG;

	err = fs_linux_create_current(&ts);
	if (err)
		return err;

	current->fs->pwd = dir->dentry;
	dget(dir->dentry);
	lerr = do_mkdir(name, mode);
	current->fs->pwd = NULL;
	dput(dir->dentry);

	fs_linux_destroy_current();

	return errno_to_oskit_error(-lerr);
}

/*
 * This is like Linux's fs/namei.c:sys_rmdir.
 */
static OSKIT_COMDECL
dir_rmdir(oskit_dir_t *d, const char *name)
{
	gdir_t *dir;
	struct task_struct ts;
	oskit_error_t err;
	int lerr;

	dir = (gdir_t *)d;
	if (!verify_dir(dir)) return OSKIT_E_INVALIDARG;

	/* No more than one component at a time. */
	if (name == NULL || strchr (name, '/') != NULL)
		return OSKIT_E_INVALIDARG;

	err = fs_linux_create_current(&ts);
	if (err)
		return err;

	current->fs->pwd = dir->dentry;
	dget(dir->dentry);
	lerr = do_rmdir(name);
	current->fs->pwd = NULL;
	dput(dir->dentry);

	fs_linux_destroy_current();

	return errno_to_oskit_error(-lerr);
}

/*
 * This is the oskit_dir_t::getdirentries code.
 *
 * getdirentries calls Linux's `readdir' directory op with a callback
 * function and some data.
 * Readdir keeps calling the callback until it returns nonzero or
 * readdir reaches the end of the directory.
 */

struct callback_state {
	struct internal_dirent *dirents;
	oskit_size_t bytesleft;
	oskit_error_t err;
};

static int
filldir(void *buf, const char *name, int namelen, off_t offset, ino_t ino)
{
	struct callback_state *state = buf;

	/* Make sure we have room. */
	if (state->bytesleft < sizeof(struct internal_dirent))
		return -EINVAL;

	/*
	 * Fill in the oskit_dirent with name and ino.
	 */
	state->dirents->name = kmalloc(namelen + 1, GFP_KERNEL);
	if (state->dirents->name == NULL) {
		state->err = OSKIT_ENOMEM;
		return -ENOMEM;
	}
	memcpy(state->dirents->name, name, namelen);
	state->dirents->name[namelen] = '\0';
	state->dirents->ino = ino;

	/* Bump state->dirents to the next one. */
	state->dirents++;
	state->bytesleft -= sizeof(struct internal_dirent);

	return 0;
}

static OSKIT_COMDECL
dir_getdirentries(oskit_dir_t *d, oskit_u32_t *inout_ofs,
		  oskit_u32_t nentries,
		  struct oskit_dirents **out_dirents)
{
	gdir_t *dir;
	struct task_struct ts;
	oskit_error_t err;
	struct file file;
	struct callback_state state;
	int lerr;
	int i, dircount;
	struct internal_dirent *dirents;

	dir = (gdir_t *)d;
	if (!verify_dir(dir)) return OSKIT_E_INVALIDARG;

	if (inout_ofs == NULL || out_dirents == NULL)
		return OSKIT_E_INVALIDARG;

	err = fs_linux_create_current(&ts);
	if (err)
		return err;

	/*
	 * Check permissions.
	 */
	lerr = permission(dir->dentry->d_inode, MAY_READ);
	if (lerr) {
		err = errno_to_oskit_error(-lerr);
		goto cleanup_current;
	}

	/*
	 * Make a fake file struct with only the fields needed.
	 */
	file.f_dentry = dir->dentry;
	file.f_pos = *inout_ofs;
	file.f_version = ++event;
	if (dir->dentry->d_inode->i_op == NULL) {
		err = OSKIT_E_INVALIDARG;
		goto cleanup_current;
	}
	file.f_op = dir->dentry->d_inode->i_op->default_file_ops;
	if (file.f_op == NULL || file.f_op->readdir == NULL) {
		err = OSKIT_ENOTDIR;
		goto cleanup_current;
	}

	/*
	 * Allocate nentries of dirents and zero them.
	 */
	dircount = 0;
	dirents  = kmalloc(nentries*sizeof(struct internal_dirent),GFP_KERNEL);
	if (dirents == NULL) {
		err = OSKIT_ENOMEM;
		goto cleanup_current;
	}
	memset(dirents, 0, nentries*sizeof(struct internal_dirent));

	/*
	 * Fill in the callback_state.
	 */
	state.dirents = dirents;
	state.bytesleft = nentries*sizeof(struct internal_dirent);
	state.err = 0;

	/*
	 * Call the `readdir' directory op.
	 */
	lerr = file.f_op->readdir(&file, &state, filldir);
	err = lerr ? errno_to_oskit_error(-lerr) : state.err;
	if (err)
		goto cleanup_dirents_current;
	*inout_ofs = file.f_pos;

	/*
	 * Figure out how many we got.
	 */
	dircount = (nentries*sizeof(struct internal_dirent)
		    - state.bytesleft) / sizeof(struct internal_dirent);
	if (dircount == 0) {
		/* Have reached EOF. */
		err = 0;
		*out_dirents = NULL;
		goto cleanup_dirents_current;
	}

	/*
	 * Create the dirents COM object first to make sure we can.
	 */
	err = fs_dirents_create(dirents, dircount, out_dirents);
	if (err) {
		*out_dirents = NULL;
		goto cleanup_dirents_current;
	}

	err = 0;
	goto cleanup_current;

 cleanup_dirents_current:
	for (i = 0; i < nentries; i++)
		if (dirents[i].name)
			kfree(dirents[i].name);
	if (err)
		kfree(dirents);
 cleanup_current:
	fs_linux_destroy_current();
	return err;
}

/*
 * This is like Linux's fs/namei.c:sys_mkdir.
 */
static OSKIT_COMDECL
dir_mknod(oskit_dir_t *d, const char *name,
	  oskit_mode_t mode, oskit_dev_t dev)
{
	gdir_t *dir;
	struct task_struct ts;
	oskit_error_t err;
	struct dentry *dentry;

	dir = (gdir_t *)d;
	if (!verify_dir(dir)) return OSKIT_E_INVALIDARG;

	/* No more than one component at a time. */
	if (name == NULL || strchr (name, '/') != NULL)
		return OSKIT_E_INVALIDARG;

	err = fs_linux_create_current(&ts);
	if (err)
		return err;

	current->fs->pwd = dir->dentry;
	dget(dir->dentry);
	dentry = do_mknod(name, (int)mode, (dev_t)dev);
	current->fs->pwd = NULL;
	dput(dir->dentry);

	if (IS_ERR(dentry)) {
		fs_linux_destroy_current();
		return errno_to_oskit_error(-(PTR_ERR(dentry)));
	}
	dput(dentry); /* Don't need it */

	fs_linux_destroy_current();

	return 0;
}

/*
 * This is like Linux's fs/namei.c:sys_symlink.
 */
static OSKIT_COMDECL
dir_symlink(oskit_dir_t *d, const char *link_name,
	    const char *dest_name)
{
	gdir_t *dir;
	struct task_struct ts;
	oskit_error_t err;
	int lerr;

	dir = (gdir_t *)d;
	if (!verify_dir(dir)) return OSKIT_E_INVALIDARG;

	if (link_name == NULL || dest_name == NULL)
		return OSKIT_E_INVALIDARG;

	/* No more than one component at a time. */
	if (strchr (link_name, '/') != NULL)
		return OSKIT_E_INVALIDARG;

	err = fs_linux_create_current(&ts);
	if (err)
		return err;

	current->fs->pwd = dir->dentry;
	dget(dir->dentry);
	lerr = do_symlink(dest_name, link_name);
	current->fs->pwd = NULL;
	dput(dir->dentry);

	fs_linux_destroy_current();

	return errno_to_oskit_error(-lerr);
}

/*
 * Create a directory object in `out_dir' that is the same as `d' but has
 * virtual parent `parent'.
 * If `parent' is NULL then the virtual parent of `out_dir' will be itself.
 */
static OSKIT_COMDECL
dir_reparent(oskit_dir_t *d, oskit_dir_t *parent,
	     oskit_dir_t **out_dir)
{
	gdir_t *dir;
	gdir_t *new_dir;
	oskit_error_t err;

	dir = (gdir_t *)d;
	if (!verify_dir(dir)) return OSKIT_E_INVALIDARG;

	if (out_dir == NULL)
		return OSKIT_E_INVALIDARG;

	err = gfile_lookup(dir->fs, dir->dentry, (gfile_t **)&new_dir);
	if (err)
		return err;
	if (parent) {
		new_dir->vparent = parent;
		oskit_dir_addref(parent);
	}
	else {
		new_dir->vparent = &new_dir->diri;
		/* No need to addref. */
	}

	*out_dir = &new_dir->diri;
	return 0;
}

static struct oskit_dir_ops dir_ops = {
	dir_query,
	(void *)file_addref,
	dir_release,
	(void *)file_stat,
	(void *)file_setstat,
	(void *)file_pathconf,
	(void *)file_sync,
	(void *)file_sync,
	(void *)file_access,
	(void *)file_readlink,
	(void *)file_open,
	(void *)file_getfs,
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
	dir_reparent,
};


/*
 * Look for the gfile in the hashtable. Search key is the dentry. Only
 * create the gfile if its not hashed, since we need to return a unique
 * COM object for the underlying filesystem object.
 */
oskit_error_t
gfile_lookup(gfilesystem_t *fs, struct dentry *dentry, gfile_t **out_file)
{
	struct gfile *file;
	oskit_error_t rc;

	file = hashtab_search(dentrytab, (hashtab_key_t) dentry);

	if (file) {
		/* gfile exists for dentry */
		oskit_file_addref(&file->filei);
	}
	else
	{
		/* no gfile for dentry; create a new one */
		rc = gfile_create(fs, dentry, &file);
		if (rc)
			return rc;
	}

	*out_file = file;
	return 0;
}

/*
 * Create and initialize a gfile OR gdir.
 * oskit_file/dir_release are the counterparts to this.
 */
oskit_error_t
gfile_create(gfilesystem_t *fs, struct dentry *dentry, gfile_t **out_file)
{
	gdir_t *dir;
	gfile_t *file;
	struct inode *inode = dentry->d_inode;
	int rc;

	/*
	 * Initialize the specific stuff.
	 */
	if (S_ISDIR(inode->i_mode)) {
		/* Create a gdir_t, which is a superset of gfile_t. */
		dir = kmalloc(sizeof(gdir_t), GFP_KERNEL);
		if (dir == NULL)
			return OSKIT_ENOMEM;
		file = (gfile_t *)dir;

		dir->diri.ops = &dir_ops;
		dir->vparent = NULL;
	}
	else {
		/* Create a gfile_t. */
		file = kmalloc(sizeof(gfile_t), GFP_KERNEL);
		if (file == NULL)
			return OSKIT_ENOMEM;
		file->filei.ops = &file_ops;
	}

	/*
	 * Add to the hashtable.
	 */
	rc = hashtab_insert(dentrytab,
			    (hashtab_key_t) dentry, (hashtab_datum_t) file);
	if (rc) {
		kfree(file);
		return OSKIT_E_FAIL;
	}

	/*
	 * Initialize the common stuff between dir and file.
	 */
	file->absioi.ops = &file_absio_ops;
	file->count = 1;
	file->dentry = dentry;
	dget(dentry);
	file->fs = fs;
	oskit_filesystem_addref(&fs->fsi);

	*out_file = file;
	return 0;
}

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
	VERIFY_OR_PANIC(fsdirents, "fs_dirents");

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
	VERIFY_OR_PANIC(fsdirents, "fs_dirents");

	return ++fsdirents->count;
}

static OSKIT_COMDECL_U
fs_dirents_release(oskit_dirents_t *d)
{
	struct fs_dirents *fsdirents = (struct fs_dirents *) d;
	int i;

	VERIFY_OR_PANIC(fsdirents, "fs_dirents");

	if (--fsdirents->count)
		return fsdirents->count;

	for (i = 0; i < fsdirents->dircount; i++)
		kfree(fsdirents->entries[i].name);

	kfree(fsdirents->entries);
	kfree(fsdirents);
	return 0;
}

static OSKIT_COMDECL
fs_dirents_getcount(oskit_dirents_t *d, int *out_count)
{
	struct fs_dirents *fsdirents = (struct fs_dirents *) d;
	VERIFY_OR_PANIC(fsdirents, "fs_dirents");

	*out_count = fsdirents->dircount;

	return 0;
}

static OSKIT_COMDECL
fs_dirents_getnext(oskit_dirents_t *d, oskit_dirent_t *dirent)
{
	struct fs_dirents	*fsdirents = (struct fs_dirents *) d;
	struct internal_dirent  *idirent;

	VERIFY_OR_PANIC(fsdirents, "fs_dirents");

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
	VERIFY_OR_PANIC(fsdirents, "fs_dirents");

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
        struct fs_dirents *fsdirents = kmalloc(sizeof(*fsdirents), GFP_KERNEL);

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
