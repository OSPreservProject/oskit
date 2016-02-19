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
 * Implements the oskit_filesystem_t COM interface.
 */

#include <oskit/io/blkio.h>
#include <oskit/fs/filesystem.h>
#include <oskit/fs/dir.h>

#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/malloc.h>

#include <assert.h>

#include "current.h"
#include "dev.h"
#include "errno.h"
#include "types.h"

static OSKIT_COMDECL
filesystem_query(oskit_filesystem_t *f,
		 const struct oskit_guid *iid,
		 void **out_ihandle)
{
	gfilesystem_t *fs;

	fs = (gfilesystem_t *)f;
	if (!verify_fs(fs, fs->sb)) return OSKIT_E_INVALIDARG;
    
	if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
	    memcmp(iid, &oskit_filesystem_iid, sizeof(*iid)) == 0) {
		*out_ihandle = &fs->fsi;
		++fs->count;
		return 0;
	}

	*out_ihandle = NULL;
	return OSKIT_E_NOINTERFACE;    
}

static OSKIT_COMDECL_U
filesystem_addref(oskit_filesystem_t *f)
{
	gfilesystem_t *fs;

	fs = (gfilesystem_t *)f;
	VERIFY_OR_PANIC(fs, "filesystem");

	return ++fs->count;
}

/*
 * This needs to undo what fs_linux_mount and gfilesystem_create do.
 */
static OSKIT_COMDECL_U
filesystem_release(oskit_filesystem_t *f)
{
	gfilesystem_t *fs;
	oskit_error_t err;
	unsigned newcount;
    
	fs = (gfilesystem_t *)f;
	VERIFY_OR_PANIC(fs, "filesystem");

	newcount = --fs->count;
	if (newcount == 0) {
		/* Temporarily bump the count so the unmount won't fail. */
		fs->count++;
		err = oskit_filesystem_unmount(f);
		assert(err == 0);
		kfree(fs);
	}

	return newcount;
}

/*
 * Obtain filesystem statistics.
 *
 * This is basically the innards of Linux's fs/open.c:sys_statfs.
 */
static OSKIT_COMDECL
filesystem_statfs(oskit_filesystem_t *f, oskit_statfs_t *out_stats)
{
	gfilesystem_t *fs;
	oskit_error_t err;
	struct statfs stats;
	struct task_struct ts;
	unsigned long flags;

	fs = (gfilesystem_t *)f;
	if (!verify_fs(fs, fs->sb)) return OSKIT_E_INVALIDARG;

	if (fs->sb->s_op->statfs == NULL)
		return OSKIT_E_INVALIDARG;

	err = fs_linux_create_current(&ts);
	if (err)
		return err;
	fs->sb->s_op->statfs(fs->sb, &stats, sizeof stats);
	fs_linux_destroy_current();

	out_stats->bsize   = stats.f_bsize;
	out_stats->frsize  = stats.f_bsize;
	out_stats->blocks  = stats.f_blocks;
	out_stats->bfree   = stats.f_bfree;
	out_stats->bavail  = stats.f_bavail;
	out_stats->files   = stats.f_files;
	out_stats->ffree   = stats.f_ffree;
	out_stats->favail  = stats.f_ffree;
	out_stats->fsid	   = stats.f_fsid.val[0];
	flags		   = fs->sb->s_flags;
	out_stats->namemax = stats.f_namelen;

	out_stats->flag = 0;
	if (flags & MS_RDONLY) out_stats->flag |= OSKIT_FS_RDONLY;
	if (flags & MS_NOEXEC) out_stats->flag |= OSKIT_FS_NOEXEC;
	if (flags & MS_NOSUID) out_stats->flag |= OSKIT_FS_NOSUID;
	if (flags & MS_NODEV)  out_stats->flag |= OSKIT_FS_NODEV;

	return 0;
}

/*
 * Sync the filesystem, optionally waiting for writes to complete.
 *
 * In Linux, sync_dev doesn't wait but fsync_dev does.
 */
static OSKIT_COMDECL
filesystem_sync(oskit_filesystem_t *f, oskit_bool_t wait)
{
	gfilesystem_t *fs;
	oskit_error_t err;
	struct task_struct ts;
	kdev_t dev;

	fs = (gfilesystem_t *)f;
	if (!verify_fs(fs, fs->sb)) return OSKIT_E_INVALIDARG;

	err = fs_linux_create_current(&ts);
	if (err)
		return err;

	dev = fs->sb->s_dev;
	if (wait && fsync_dev(dev) != 0)
		/* fsync_dev can return 1 if there was an IO error. */
		return OSKIT_EIO;
	else
		sync_dev(dev);

	fs_linux_destroy_current();

	return 0;
}

/*
 * Get a handle to the root dir.
 *
 * Linux stores a pointer to the dentry of the root dir in the
 * super_block struct as s_root.
 */
static OSKIT_COMDECL
filesystem_getroot(oskit_filesystem_t *f, oskit_dir_t **out_dir)
{
	gfilesystem_t *fs;
	gdir_t *dir;
	oskit_error_t err;

	fs = (gfilesystem_t *)f;
	if (!verify_fs(fs, fs->sb)) return OSKIT_E_INVALIDARG;

	if (out_dir == NULL)
		return OSKIT_E_INVALIDARG;

	err = gfile_lookup(fs, fs->sb->s_root, (gfile_t **)&dir);
	if (err)
		return err;
	*out_dir = &dir->diri;

	return 0;
}

/*
 * Remount the filesystem.
 *
 * This is basically the innards of Linux's fs/super.c:do_remount_sb.
 */
static OSKIT_COMDECL
filesystem_remount(oskit_filesystem_t *f, oskit_u32_t flags)
{
	gfilesystem_t *fs;
	oskit_error_t err;
	struct task_struct ts;
	struct super_block *sb;
	int linuxerr;

	fs = (gfilesystem_t *)f;
	if (!verify_fs(fs, fs->sb)) return OSKIT_E_INVALIDARG;

	err = fs_linux_create_current(&ts);
	if (err)
		return err;
	
	sb = fs->sb;
	if (sb->s_op && sb->s_op->remount_fs) {
		linuxerr = sb->s_op->remount_fs(sb, &flags, 0);
		if (linuxerr)
			return errno_to_oskit_error(-linuxerr);
	}
	sb->s_flags = (sb->s_flags & ~MS_RMT_MASK) |
		(flags & MS_RMT_MASK);

	fs_linux_destroy_current();

	return 0;
}

/*
 * Unmount the filesystem regardless of if there are still references to us.
 * This is like shortcutting oskit_fileystem_release and is discouraged.
 * Any references to this fs or files within it after unmount will yield
 * undefined results.
 *
 * This is intended to be like Linux's fs/super.c:sys_umount and do_umount,
 * mostly the latter.
 */
static OSKIT_COMDECL
filesystem_unmount(oskit_filesystem_t *f)
{
	gfilesystem_t *fs;
	oskit_error_t err;
	struct task_struct ts;
	struct super_block *sb;
	kdev_t dev;

	fs = (gfilesystem_t *)f;
	if (!verify_fs(fs, fs->sb)) return OSKIT_E_INVALIDARG;

	err = fs_linux_create_current(&ts);
	if (err)
		return err;

	sb = fs->sb;
	dev = sb->s_dev;

	if (sb->s_op->umount_begin)
		sb->s_op->umount_begin(sb);

	/*
	 * Shrink dcache, then fsync. This guarantees that if the
	 * filesystem is quiescent at this point, then (a) only the
	 * root entry should be in use and (b) that root entry is
	 * clean.
	 */
	shrink_dcache_sb(sb);
	fsync_dev(dev);

	err = d_umount(sb);
	if (err)
		return err;

	if (sb->s_op) {
		if (sb->s_op->put_super)
			sb->s_op->put_super(sb);
	}

	/* Forget any remaining inodes */
	if (invalidate_inodes(sb)) {
		printk("VFS: Busy inodes after unmount. "
			"Self-destruct in 5 seconds.  Have a nice day...\n");
	}

	sb->s_dev = 0;		/* Free the superblock */

	fsync_dev(dev);

	fs_linux_devtab_delete(dev);
	fs_linux_destroy_current();

	return 0;
}

static OSKIT_COMDECL
filesystem_lookupi(oskit_filesystem_t *f, oskit_ino_t ino,
		   oskit_file_t **out_file)
{
	gfilesystem_t *fs;
	oskit_error_t err = 0;
	struct inode *inode;
	struct dentry *dentry;
	struct task_struct ts;
	gfile_t *file;

	fs = (gfilesystem_t *)f;
	if (!verify_fs(fs, fs->sb)) return OSKIT_E_INVALIDARG;

/**/	err = fs_linux_create_current(&ts);
	if (err)
		return err;

	/* Get the dentry. */
/**/	inode = iget(fs->sb, ino);
	if (inode == NULL) {
		fs_linux_destroy_current();
		return OSKIT_E_FAIL;
	}

	/* Create a dentry. This is kinda bogus, but so is lookup by inode */
/**/	dentry = d_alloc(NULL, &(const struct qstr) { "", 0, 0 });
	if (dentry == NULL) {
		iput(inode);
		fs_linux_destroy_current();
		return OSKIT_E_FAIL;
	}
	d_instantiate(dentry, inode);
	dentry->d_sb = inode->i_sb;

	/* Make a file from the dentry. */
/**/	err = gfile_lookup(fs, dentry, &file);
	if (err) {
		iput(inode);
		fs_linux_destroy_current();
		return err;
	}
	*out_file = &file->filei;

	iput(inode);			/* `file' has a ref */
	fs_linux_destroy_current();
	return err;
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
 * Create and initialize a gfilesystem_t.
 */
static oskit_error_t
gfilesystem_create(struct super_block *sb, gfilesystem_t **out_fs)
{
	gfilesystem_t *fs;

	if (out_fs == NULL)
		return OSKIT_E_INVALIDARG;

	fs = kmalloc(sizeof(gfilesystem_t), GFP_KERNEL);
	if (fs == NULL)
		return OSKIT_ENOMEM;
	fs->fsi.ops = &fs_ops;
	fs->count = 1;
	fs->sb = sb;
	*out_fs = fs;

	return 0;

}

/*
 * This is a front end for Linux's mount_root,
 * which fills in a super_block struct based on what it finds in the partition.
 *
 * oskit_filesystem_release is the counterpart to this.
 */
oskit_error_t
fs_linux_mount(oskit_blkio_t *bio, oskit_u32_t flags,
	       oskit_filesystem_t **out_fs)
{
	int linux_flags;
	kdev_t dev;
	struct super_block *sb;
	int linux_err;
	struct task_struct ts;
	oskit_error_t err;
	gfilesystem_t *fs;

	if (out_fs == NULL)
		return OSKIT_E_INVALIDARG;

	/*
	 * Insert this device into the devtab and get a kdev_t for it.
	 */
	err = fs_linux_devtab_insert(bio, &dev);
	if (err)
		return err;

	/*
	 * Convert `flags' into Linux form.
	 */
	linux_flags = 0;
	if (flags & OSKIT_FS_RDONLY) linux_flags |= MS_RDONLY;
	if (flags & OSKIT_FS_NOEXEC) linux_flags |= MS_NOEXEC;
	if (flags & OSKIT_FS_NOSUID) linux_flags |= MS_NOSUID;
	if (flags & OSKIT_FS_NODEV)  linux_flags |= MS_NODEV;

	/*
	 * Call a modified version of Linux's mount_root that takes
	 * parameters rather then using ROOT_DEV and root_mountflags.
	 */
	err = fs_linux_create_current(&ts);
	if (err) {
		fs_linux_devtab_delete(dev);
		return err;
	}
	linux_err = mount_root(dev, linux_flags, &sb);
	if (linux_err) {
		printk("%s: Linux mount_root failed %d\n",
		       __FUNCTION__, linux_err);
		fs_linux_devtab_delete(dev);
		fs_linux_destroy_current();
		return OSKIT_EIO;
	}
	fs_linux_destroy_current();

	/*
	 * Now, create something to return.
	 */
	err = gfilesystem_create(sb, &fs);
	if (err) {
		/* XXX need to split unmount functionality out of
                   filesystem_unmount and do that here.
		   Will still need current to be valid. */
		fs_linux_devtab_delete(dev);
		return err;
	}
	*out_fs = &fs->fsi;

	return 0;
}

