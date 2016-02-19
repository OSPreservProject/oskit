/*
 *  linux/fs/ext2/file.c
 *
 * Copyright (C) 1992, 1993, 1994, 1995
 * Remy Card (card@masi.ibp.fr)
 * Laboratoire MASI - Institut Blaise Pascal
 * Universite Pierre et Marie Curie (Paris VI)
 *
 *  from
 *
 *  linux/fs/minix/file.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 *
 *  ext2 fs regular file handling primitives
 *
 *  64-bit file support on 64-bit platforms by Jakub Jelinek
 * 	(jj@sunsite.ms.mff.cuni.cz)
 */

#include <asm/uaccess.h>
#include <asm/system.h>

#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/ext2_fs.h>
#include <linux/fcntl.h>
#include <linux/sched.h>
#include <linux/stat.h>
#include <linux/locks.h>
#include <linux/mm.h>
#include <linux/pagemap.h>

#define	NBUF	32

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

static long long ext2_file_lseek(struct file *, long long, int);
static ssize_t ext2_file_write (struct file *, const char *, size_t, loff_t *);
static int ext2_release_file (struct inode *, struct file *);
#if BITS_PER_LONG < 64
static int ext2_open_file (struct inode *, struct file *);

#else

#define EXT2_MAX_SIZE(bits)							\
	(((EXT2_NDIR_BLOCKS + (1LL << (bits - 2)) + 				\
	   (1LL << (bits - 2)) * (1LL << (bits - 2)) + 				\
	   (1LL << (bits - 2)) * (1LL << (bits - 2)) * (1LL << (bits - 2))) * 	\
	  (1LL << bits)) - 1)

long long ext2_max_sizes[] = {
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
EXT2_MAX_SIZE(10), EXT2_MAX_SIZE(11), EXT2_MAX_SIZE(12), EXT2_MAX_SIZE(13)
};

#endif

/*
 * We have mostly NULL's here: the current defaults are ok for
 * the ext2 filesystem.
 */
static struct file_operations ext2_file_operations = {
	ext2_file_lseek,	/* lseek */
	generic_file_read,	/* read */
	ext2_file_write,	/* write */
	NULL,			/* readdir - bad */
	NULL,			/* poll - default */
	ext2_ioctl,		/* ioctl */
	generic_file_mmap,	/* mmap */
#if BITS_PER_LONG == 64	
	NULL,			/* no special open is needed */
#else
	ext2_open_file,
#endif
	NULL,			/* flush */
	ext2_release_file,	/* release */
	ext2_sync_file,		/* fsync */
	NULL,			/* fasync */
	NULL,			/* check_media_change */
	NULL			/* revalidate */
};

struct inode_operations ext2_file_inode_operations = {
	&ext2_file_operations,/* default file operations */
	NULL,			/* create */
	NULL,			/* lookup */
	NULL,			/* link */
	NULL,			/* unlink */
	NULL,			/* symlink */
	NULL,			/* mkdir */
	NULL,			/* rmdir */
	NULL,			/* mknod */
	NULL,			/* rename */
	NULL,			/* readlink */
	NULL,			/* follow_link */
	generic_readpage,	/* readpage */
	NULL,			/* writepage */
	ext2_bmap,		/* bmap */
	ext2_truncate,		/* truncate */
	ext2_permission,	/* permission */
	NULL			/* smap */
};

/*
 * Make sure the offset never goes beyond the 32-bit mark..
 */
static long long ext2_file_lseek(
	struct file *file,
	long long offset,
	int origin)
{
	struct inode *inode = file->f_dentry->d_inode;

	switch (origin) {
		case 2:
			offset += inode->i_size;
			break;
		case 1:
			offset += file->f_pos;
	}
	if (((unsigned long long) offset >> 32) != 0) {
#if BITS_PER_LONG < 64
		return -EINVAL;
#else
		if (offset > ext2_max_sizes[EXT2_BLOCK_SIZE_BITS(inode->i_sb)])
			return -EINVAL;
#endif
	} 
	if (offset != file->f_pos) {
		file->f_pos = offset;
		file->f_reada = 0;
		file->f_version = ++event;
	}
	return offset;
}

static inline void remove_suid(struct inode *inode)
{
	unsigned int mode;

	/* set S_IGID if S_IXGRP is set, and always set S_ISUID */
	mode = (inode->i_mode & S_IXGRP)*(S_ISGID/S_IXGRP) | S_ISUID;

	/* was any of the uid bits set? */
	mode &= inode->i_mode;
	if (mode && !capable(CAP_FSETID)) {
		inode->i_mode &= ~mode;
		mark_inode_dirty(inode);
	}
}

static ssize_t ext2_file_write (struct file * filp, const char * buf,
				size_t count, loff_t *ppos)
{
	struct inode * inode = filp->f_dentry->d_inode;
	off_t pos;
	long block;
	int offset;
	int written, c;
	struct buffer_head * bh, *bufferlist[NBUF];
	struct super_block * sb;
	int err;
	int i,buffercount,write_error, new_buffer;
	unsigned long limit;
	
	/* POSIX: mtime/ctime may not change for 0 count */
	if (!count)
		return 0;
	/* This makes the bounds-checking arithmetic later on much more
	 * sane. */
	if (((signed) count) < 0)
		return -EINVAL;
	
	write_error = buffercount = 0;
	if (!inode) {
		printk("ext2_file_write: inode = NULL\n");
		return -EINVAL;
	}
	sb = inode->i_sb;
	if (sb->s_flags & MS_RDONLY)
		/*
		 * This fs has been automatically remounted ro because of errors
		 */
		return -ENOSPC;

	if (!S_ISREG(inode->i_mode)) {
		ext2_warning (sb, "ext2_file_write", "mode = %07o",
			      inode->i_mode);
		return -EINVAL;
	}
	remove_suid(inode);

	if (filp->f_flags & O_APPEND)
		pos = inode->i_size;
	else {
		pos = *ppos;
		if (pos != *ppos)
			return -EINVAL;
	}

	/* Check for overflow.. */

#if BITS_PER_LONG < 64
	/* If the fd's pos is already greater than or equal to the file
	 * descriptor's offset maximum, then we need to return EFBIG for
	 * any non-zero count (and we already tested for zero above). */
	if (((unsigned) pos) >= 0x7FFFFFFFUL)
		return -EFBIG;
	
	/* If we are about to overflow the maximum file size, we also
	 * need to return the error, but only if no bytes can be written
	 * successfully. */
	if (((unsigned) pos + count) > 0x7FFFFFFFUL) {
		count = 0x7FFFFFFFL - pos;
		if (((signed) count) < 0)
			return -EFBIG;
	}
#else
	{
		off_t max = ext2_max_sizes[EXT2_BLOCK_SIZE_BITS(sb)];

		if (pos >= max)
			return -EFBIG;
		
		if (pos + count > max) {
			count = max - pos;
			if (!count)
				return -EFBIG;
		}
		if (((pos + count) >> 31) && 
		    !(sb->u.ext2_sb.s_es->s_feature_ro_compat &
		      cpu_to_le32(EXT2_FEATURE_RO_COMPAT_LARGE_FILE))) {
			/* If this is the first large file created, add a flag
			   to the superblock */
			sb->u.ext2_sb.s_es->s_feature_ro_compat |=
				cpu_to_le32(EXT2_FEATURE_RO_COMPAT_LARGE_FILE);
			mark_buffer_dirty(sb->u.ext2_sb.s_sbh, 1);
		}
	}
#endif

	/* From SUS: We must generate a SIGXFSZ for file size overflow
	 * only if no bytes were actually written to the file. --sct */

	limit = current->rlim[RLIMIT_FSIZE].rlim_cur;
	if (limit < RLIM_INFINITY) {
		if (((unsigned) pos+count) >= limit) {
			count = limit - pos;
			if (((signed) count) <= 0) {
				send_sig(SIGXFSZ, current, 0);
				return -EFBIG;
			}
		}
	}

	/*
	 * If a file has been opened in synchronous mode, we have to ensure
	 * that meta-data will also be written synchronously.  Thus, we
	 * set the i_osync field.  This field is tested by the allocation
	 * routines.
	 */
	if (filp->f_flags & O_SYNC)
		inode->u.ext2_i.i_osync++;
	block = pos >> EXT2_BLOCK_SIZE_BITS(sb);
	offset = pos & (sb->s_blocksize - 1);
	c = sb->s_blocksize - offset;
	written = 0;
	do {
		bh = ext2_getblk (inode, block, 1, &err);
		if (!bh) {
			if (!written)
				written = err;
			break;
		}
		if (c > count)
			c = count;

		/* Tricky: what happens if we are writing the complete
		 * contents of a block which is not currently
		 * initialised?  We have to obey the same
		 * synchronisation rules as the IO code, to prevent some
		 * other process from stomping on the buffer contents by
		 * refreshing them from disk while we are setting up the
		 * buffer.  The copy_from_user() can page fault, after
		 * all.  We also have to throw away partially successful
		 * copy_from_users to such buffers, since we can't trust
		 * the rest of the buffer_head in that case.  --sct */

		new_buffer = (!buffer_uptodate(bh) && !buffer_locked(bh) &&
			      c == sb->s_blocksize);

		if (new_buffer) {
			set_bit(BH_Lock, &bh->b_state);
			c -= copy_from_user (bh->b_data + offset, buf, c);
			if (c != sb->s_blocksize) {
				c = 0;
				unlock_buffer(bh);
				brelse(bh);
				if (!written)
					written = -EFAULT;
				break;
			}
			mark_buffer_uptodate(bh, 1);
			unlock_buffer(bh);
		} else {
			if (!buffer_uptodate(bh)) {
				ll_rw_block (READ, 1, &bh);
				wait_on_buffer (bh);
				if (!buffer_uptodate(bh)) {
					brelse (bh);
					if (!written)
						written = -EIO;
					break;
				}
			}
			c -= copy_from_user (bh->b_data + offset, buf, c);
		}
		if (!c) {
			brelse(bh);
			if (!written)
				written = -EFAULT;
			break;
		}
		mark_buffer_dirty(bh, 0);
		update_vm_cache(inode, pos, bh->b_data + offset, c);
		pos += c;
		written += c;
		buf += c;
		count -= c;

		if (filp->f_flags & O_SYNC)
			bufferlist[buffercount++] = bh;
		else
			brelse(bh);
		if (buffercount == NBUF){
			ll_rw_block(WRITE, buffercount, bufferlist);
			for(i=0; i<buffercount; i++){
				wait_on_buffer(bufferlist[i]);
				if (!buffer_uptodate(bufferlist[i]))
					write_error=1;
				brelse(bufferlist[i]);
			}
			buffercount=0;
		}
		if(write_error)
			break;
		block++;
		offset = 0;
		c = sb->s_blocksize;
	} while (count);
	if ( buffercount ){
		ll_rw_block(WRITE, buffercount, bufferlist);
		for(i=0; i<buffercount; i++){
			wait_on_buffer(bufferlist[i]);
			if (!buffer_uptodate(bufferlist[i]))
				write_error=1;
			brelse(bufferlist[i]);
		}
	}		
	if (pos > inode->i_size)
		inode->i_size = pos;
	if (filp->f_flags & O_SYNC)
		inode->u.ext2_i.i_osync--;
	inode->i_ctime = inode->i_mtime = CURRENT_TIME;
	*ppos = pos;
	mark_inode_dirty(inode);
	return written;
}

/*
 * Called when an inode is released. Note that this is different
 * from ext2_file_open: open gets called at every open, but release
 * gets called only when /all/ the files are closed.
 */
static int ext2_release_file (struct inode * inode, struct file * filp)
{
	if (filp->f_mode & FMODE_WRITE)
		ext2_discard_prealloc (inode);
	return 0;
}

#if BITS_PER_LONG < 64
/*
 * Called when an inode is about to be open.
 * We use this to disallow opening RW large files on 32bit systems.
 */
static int ext2_open_file (struct inode * inode, struct file * filp)
{
	if (inode->u.ext2_i.i_high_size && (filp->f_mode & FMODE_WRITE))
		return -EFBIG;
	return 0;
}
#endif
