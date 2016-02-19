/*
 * Copyright (c) 1995, 1997, 1998 University of Utah and the Flux Group.
 * All rights reserved.
 * 
 * This file is part of the OSKit Filesystem Reading Library, which is free
 * software, also known as "open source;" you can redistribute it and/or modify
 * it under the terms of the GNU General Public License (GPL), version 2, as
 * published by the Free Software Foundation (FSF).
 * 
 * The OSKit is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GPL for more details.  You should have
 * received a copy of the GPL along with the OSKit; see the file COPYING.  If
 * not, write to the FSF, 59 Temple Place #330, Boston, MA 02111-1307, USA.
 */

/* 
 * Mach Operating System
 * Copyright (c) 1991,1990 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
/*
 * Stand-alone file reading package.
 */

#include <string.h>
#include <stdlib.h>	/* malloc, free */
#include <oskit/io/blkio.h>
#include <oskit/fs/read.h>


/*** Stuff that was in defs.h ***/

typedef	unsigned char	u_char;		/* unsigned char */
typedef	unsigned short	u_short;	/* unsigned short */
typedef	unsigned int	u_int;		/* unsigned int */

typedef struct _quad_ {
	unsigned int	val[2];		/* 2 int values make... */
} quad;					/* an 8-byte item */

typedef	unsigned int	time_t;		/* an unsigned int */
typedef unsigned int	daddr_t;	/* an unsigned int */
typedef	unsigned int	off_t;		/* another unsigned int */

typedef	unsigned short	uid_t;
typedef	unsigned short	gid_t;
typedef	unsigned int	ino_t;

#define	MAXBSIZE	8192
#define	MAXFRAG		8

#define	MAXPATHLEN	1024
#define	MAXSYMLINKS	8


#include "ffs_fs.h"
#include "ffs_dir.h"
#include "disk_inode.h"
#include "disk_inode_ffs.h"
#include "common.h"

#define	FS_NOT_DIRECTORY	OSKIT_ENOTDIR		/* not a directory */
#define	FS_NO_ENTRY		OSKIT_ENOENT		/* name not found */
#define	FS_NAME_TOO_LONG	OSKIT_ENAMETOOLONG	/* name too long */
#define	FS_SYMLINK_LOOP		OSKIT_ELOOP		/* symbolic link loop */
#define	FS_INVALID_FS		OSKIT_EINVAL		/* bad file system */
#define	FS_NOT_IN_FILE		OSKIT_EINVAL		/* offset not in file */
#define	FS_INVALID_PARAMETER	OSKIT_EINVAL		/* bad parameter to
							   a routine */

/*** Stuff that was in file_io.h ***/

/*
 * In-core open file.
 */
struct file {
	oskit_blkio_t		f_bioi;		/* interface to this file */
	unsigned		f_refs;		/* reference count */

	oskit_blkio_t		*f_blkio;	/* underlying device handle */

	oskit_addr_t		f_buf;		/* buffer for data block */
	daddr_t			f_buf_blkno;	/* block number of data block */

	/*
	 * This union no longer has any purpose;
	 * it's just a vestigial hack carried over from the Mach code.
	 */
	union {
		struct {
			struct fs *	ffs_fs;	/* pointer to super-block */
			struct icommon	ffs_ic;	/* copy of on-disk inode */

			/* number of blocks mapped by
			   indirect block at level i */
			int			ffs_nindir[FFS_NIADDR+1];

			/* buffer for indirect block at level i */
			oskit_addr_t		ffs_blk[FFS_NIADDR];

			/* size of buffer */
			oskit_size_t		ffs_blksize[FFS_NIADDR];

			/* disk address of block in buffer */
			daddr_t			ffs_blkno[FFS_NIADDR];
		} ffs;
	} u;
};


/*
 * Free file buffers, but don't close file.
 */
static void
free_file_buffers(fp)
	register struct file	*fp;
{
	register int level;

	/*
	 * Free the indirect blocks
	 */
	for (level = 0; level < NIADDR; level++) {
	    if (fp->f_blk[level] != 0) {
	    	free((void*)fp->f_blk[level]);
		fp->f_blk[level] = 0;
	    }
	    fp->f_blkno[level] = -1;
	}

	/*
	 * Free the data block
	 */
	if (fp->f_buf != 0) {
	    free((void*)fp->f_buf);
	    fp->f_buf = 0;
	}
	fp->f_buf_blkno = -1;
}

/*
 * Read a new inode into a file structure.
 */
static int
read_inode(inumber, fp)
	ino_t			inumber;
	register struct file	*fp;
{
	struct fs		*fs = fp->f_fs;
	oskit_size_t		buf_size = fs->fs_bsize;
	char			buf[buf_size];
	daddr_t			disk_block;
	oskit_error_t		rc;
	oskit_size_t		actual;

	disk_block = fsbtodb(fs, itod(fs, inumber));

	rc = oskit_blkio_read(fp->f_blkio, buf,
		(oskit_off_t)disk_block * DEV_BSIZE, buf_size, &actual);
	if (rc != 0)
		return rc;
	if (actual != buf_size)
		return OSKIT_E_UNEXPECTED; /* XXX bad FS format */

	{
	    register struct dinode *dp;

	    dp = (struct dinode *)buf;
	    dp += itoo(fs, inumber);
	    fp->i_ic = dp->di_ic;
	}

	/*
	 * Clear out the old buffers
	 */
	free_file_buffers(fp);

	return (0);	 
}

/*
 * Given an offset in a file, find the disk block number that
 * contains that block.
 */
static int
block_map(fp, file_block, disk_block_p)
	struct file	*fp;
	daddr_t		file_block;
	daddr_t		*disk_block_p;	/* out */
{
	int		level;
	int		idx;
	daddr_t		ind_block_num;
	struct fs	*fs = fp->f_fs;
	oskit_error_t	rc;

	/*
	 * Index structure of an inode:
	 *
	 * i_db[0..NDADDR-1]	hold block numbers for blocks
	 *			0..NDADDR-1
	 *
	 * i_ib[0]		index block 0 is the single indirect
	 *			block
	 *			holds block numbers for blocks
	 *			NDADDR .. NDADDR + NINDIR(fs)-1
	 *
	 * i_ib[1]		index block 1 is the double indirect
	 *			block
	 *			holds block numbers for INDEX blocks
	 *			for blocks
	 *			NDADDR + NINDIR(fs) ..
	 *			NDADDR + NINDIR(fs) + NINDIR(fs)**2 - 1
	 *
	 * i_ib[2]		index block 2 is the triple indirect
	 *			block
	 *			holds block numbers for double-indirect
	 *			blocks for blocks
	 *			NDADDR + NINDIR(fs) + NINDIR(fs)**2 ..
	 *			NDADDR + NINDIR(fs) + NINDIR(fs)**2
	 *				+ NINDIR(fs)**3 - 1
	 */

	if (file_block < NDADDR) {
	    /* Direct block. */
	    *disk_block_p = fsbtodb(fs, fp->i_db[file_block]);
	    return (0);
	}

	file_block -= NDADDR;

	/*
	 * nindir[0] = NINDIR
	 * nindir[1] = NINDIR**2
	 * nindir[2] = NINDIR**3
	 *	etc
	 */
	for (level = 0; level < NIADDR; level++) {
	    if (file_block < fp->f_nindir[level])
		break;
	    file_block -= fp->f_nindir[level];
	}
	if (level == NIADDR) {
	    /* Block number too high */
	    return (FS_NOT_IN_FILE);
	}

	ind_block_num = fp->i_ib[level];

	for (; level >= 0; level--) {

	    oskit_size_t	size = fp->f_fs->fs_bsize;
	    oskit_size_t	actual;

	    if (ind_block_num == 0)
		break;

	    if (fp->f_blkno[level] != ind_block_num) {
		/*
		 * Cache miss - need to read a different block.
		 */

		/* Allocate the block buffer if necessary */
		if (fp->f_blk[level] == 0) {
			fp->f_blk[level] = (oskit_addr_t)malloc(size);
			if (fp->f_blk[level] == 0)
				return OSKIT_E_OUTOFMEMORY;
		}

		/* Read the block */
		rc = oskit_blkio_read(fp->f_blkio,
			(void*)fp->f_blk[level],
			(oskit_off_t)fsbtodb(fs, ind_block_num) * DEV_BSIZE, size, &actual);
		if (rc != 0)
		    return rc;
		if (actual != size)
		    return OSKIT_E_UNEXPECTED; /* XXX bad fs */

		/* Remember which block we've got cached */
		fp->f_blkno[level] = ind_block_num;
	    }

	    if (level > 0) {
		idx = file_block / fp->f_nindir[level-1];
		file_block %= fp->f_nindir[level-1];
	    }
	    else
		idx = file_block;

	    ind_block_num = ((daddr_t *)fp->f_blk[level])[idx];
	}

	*disk_block_p = fsbtodb(fs, ind_block_num);
	return (0);
}

/*
 * Read a portion of a file into an internal buffer.  Return
 * the location in the buffer and the amount in the buffer.
 */
static int
buf_read_file(fp, offset, buf_p, size_p)
	register struct file	*fp;
	oskit_addr_t		offset;
	oskit_addr_t		*buf_p;		/* out */
	oskit_size_t		*size_p;	/* out */
{
	register struct fs	*fs;
	oskit_addr_t		off;
	register daddr_t	file_block;
	daddr_t			disk_block;
	int			rc;
	oskit_addr_t		block_size;

	if (offset >= fp->i_size) {
	    *size_p = 0;
	    return 0;
	}

	fs = fp->f_fs;

	off = blkoff(fs, offset);
	file_block = lblkno(fs, offset);
	block_size = blksize(fs, fp, file_block);

	if (file_block != fp->f_buf_blkno) {
	    rc = block_map(fp, file_block, &disk_block);
	    if (rc != 0)
		return (rc);

	    /* Allocate the buffer if necessary */
	    if (fp->f_buf == 0) {
	    	fp->f_buf = (oskit_addr_t)malloc(block_size);
		if (fp->f_buf == 0)
		    return OSKIT_E_OUTOFMEMORY;
	    }

	    /* Read the appropriate data into the buffer */
	    if (disk_block == 0) {
	    	memset((void*)fp->f_buf, 0, block_size);
	    } else {
	    	oskit_size_t actual;
	    	rc = oskit_blkio_read(fp->f_blkio, (void*)fp->f_buf,
					    (oskit_off_t)disk_block * DEV_BSIZE,
					    block_size, &actual);
		if (rc)
			return rc;
		if (actual != block_size)
			return OSKIT_E_UNEXPECTED;
	    }

	    /* Remember which block this is */
	    fp->f_buf_blkno = file_block;
	}

	/*
	 * Return address of byte in buffer corresponding to
	 * offset, and size of remainder of buffer after that
	 * byte.
	 */
	*buf_p = fp->f_buf + off;
	*size_p = block_size - off;

	/*
	 * But truncate buffer at end of file.
	 */
	if (*size_p > fp->i_size - offset)
	    *size_p = fp->i_size - offset;

	return (0);
}

/* In 4.4 d_reclen is split into d_type and d_namlen */
struct dirent_44 {
	unsigned long	d_fileno;
	unsigned short	d_reclen;
	unsigned char	d_type;
	unsigned char	d_namlen;
	char		d_name[255 + 1];
};

/*
 * Search a directory for a name and return its
 * i_number.
 */
static int
search_directory(name, fp, inumber_p)
	char *		name;
	register struct file *fp;
	ino_t		*inumber_p;	/* out */
{
	oskit_addr_t	buf;
	oskit_size_t	buf_size;
	oskit_addr_t	offset;
	register struct dirent_44 *dp;
	int		length;
	oskit_error_t	rc;

	length = strlen(name);

	offset = 0;
	while (offset < fp->i_size) {
	    rc = buf_read_file(fp, offset, &buf, &buf_size);
	    if (rc != 0)
		return (rc);
	    if (buf_size == 0)
		break;

	    dp = (struct dirent_44 *)buf;
	    if (dp->d_ino != 0) {
		unsigned short namlen = dp->d_namlen;
		/* 
		 * If namlen is zero, then either this is a 4.3 file
		 * system or the namlen is really zero.  In the latter
		 * case also the 4.3 d_namlen field is zero
		 * interpreted either way.
		 */
		if (namlen == 0)
		    namlen = ((struct direct *)dp)->d_namlen;
                   
		if (namlen == length &&
		    !strcmp(name, dp->d_name))
	    	{
		    /* found entry */
		    *inumber_p = dp->d_ino;
		    return (0);
		}
	    }
	    offset += dp->d_reclen;
	}
	return (FS_NO_ENTRY);
}

static int
read_fs(bio, fsp)
	oskit_blkio_t *bio;
	struct fs **fsp;
{
	register struct fs *fs;
	oskit_addr_t	actual;
	int		error;

	fs = malloc(SBSIZE);
	if (fs == NULL)
		return OSKIT_E_OUTOFMEMORY;
	error = oskit_blkio_read(bio, fs, SBLOCK * DEV_BSIZE, SBSIZE, &actual);
	if (error) {
	    free(fs);
	    return (error);
	}
	if (actual != SBSIZE) {
	    free(fs);
	    return FS_INVALID_FS;
	}

	/*
	 * Check the superblock
	 */
	if (fs->fs_magic != FS_MAGIC ||
	    fs->fs_bsize > MAXBSIZE ||
	    fs->fs_bsize < sizeof(struct fs)) {
		free(fs);
		return (FS_INVALID_FS);
	}
	/* don't read cylinder groups - we aren't modifying anything */

	*fsp = fs;
	return 0;
}

static int
mount_fs(fp)
	register struct file	*fp;
{
	register struct fs *fs;
	int error;

	error = read_fs(fp->f_blkio, &fp->f_fs);
	if (error)
	    return (error);
	fs = fp->f_fs;

	/*
	 * Calculate indirect block levels.
	 */
	{
	    register int	mult;
	    register int	level;

	    mult = 1;
	    for (level = 0; level < NIADDR; level++) {
		mult *= NINDIR(fs);
		fp->f_nindir[level] = mult;
	    }
	}

	return (0);
}

static void
unmount_fs(fp)
	register struct file	*fp;
{
	if (fp->f_fs) {
	    free(fp->f_fs);
	    fp->f_fs = 0;
	}
}


/*
 * Close file - free all storage used after ref count drops to 0.
 */
static OSKIT_COMDECL_U
ffs_release(oskit_blkio_t *io)
{
	struct file *fp = (struct file*)io;

	if (--fp->f_refs > 0)
		return fp->f_refs;

	/*
	 * Free the disk super-block.
	 */
	unmount_fs(fp);

	/*
	 * Free the inode and data buffers.
	 */
	free_file_buffers(fp);

	/* Release our reference to the underlying device.  */
	oskit_blkio_release(fp->f_blkio);

	return 0;
}

/*
 * Copy a portion of a file into kernel memory.
 * Cross block boundaries when necessary.
 */
static OSKIT_COMDECL
ffs_read(oskit_blkio_t *io, void *start, oskit_off_t offset, oskit_size_t size,
	 oskit_size_t *out_actual)
{
	struct file		*fp = (struct file*)io;
	int			rc;
	register oskit_size_t	csize;
	oskit_addr_t		buf;
	oskit_size_t		buf_size;
	oskit_size_t		actual = 0;

	while (size != 0) {
	    rc = buf_read_file(fp, (oskit_addr_t)offset, &buf, &buf_size);
	    if (rc)
		return (rc);

	    csize = size;
	    if (csize > buf_size)
		csize = buf_size;
	    if (csize == 0)
		break;

	    memcpy(start, (char *)buf, csize);

	    actual += csize;
	    offset += csize;
	    start  += csize;
	    size   -= csize;
	}

	*out_actual = actual;
	return (0);
}

static OSKIT_COMDECL
ffs_getsize(oskit_blkio_t *io, oskit_off_t *out_size)
{
	struct file *fp = (struct file*)io;

	*out_size = fp->i_size;
	return 0;
}

static struct oskit_blkio_ops biops = {
	fsread_common_query,
	fsread_common_addref,
	ffs_release,
	fsread_common_getblocksize,
	ffs_read,
	fsread_common_write,
	ffs_getsize,
	fsread_common_setsize
};


/*
 * Open a file.
 */
oskit_error_t
fsread_ffs_open(oskit_blkio_t *bio, const char *path, oskit_blkio_t **out_fbio)
{
#define	RETURN(code)	{ rc = (code); goto exit; }

	register char	*cp, *component;
	register int	c;	/* char */
	register int	rc;
	ino_t		inumber, parent_inumber;
	int		nlinks = 0;
	struct file	*fp;
	char	namebuf[MAXPATHLEN+1];
	
	if (path == 0)
		path = "";

	/*
	 * Copy name into buffer to allow modifying it.
	 */
	strcpy(namebuf, path);
	cp = namebuf;

	/* Create the file structure */
	fp = calloc(sizeof(*fp), 1);
	if (fp == NULL)
		return OSKIT_E_OUTOFMEMORY;
	fp->f_bioi.ops = &biops;
	fp->f_refs = 1;
	fp->f_blkio = bio;
	oskit_blkio_addref(bio);

	/* Mount the file system */
	rc = mount_fs(fp);
	if (rc)
	    goto exit;

	inumber = (ino_t) ROOTINO;
	if ((rc = read_inode(inumber, fp)) != 0) {
	    goto exit;
	}

	while (*cp) {

	    /*
	     * Check that current node is a directory.
	     */
	    if ((fp->i_mode & IFMT) != IFDIR)
		RETURN (FS_NOT_DIRECTORY);

	    /*
	     * Remove extra separators
	     */
	    while (*cp == '/')
		cp++;

	    /*
	     * Get next component of path name.
	     */
	    component = cp;
	    {
		register int	len = 0;

		while ((c = *cp) != '\0' && c != '/') {
		    if (len++ > MAXNAMLEN)
			RETURN (FS_NAME_TOO_LONG);
		    if (c & 0200)
			RETURN (FS_INVALID_PARAMETER);
		    cp++;
		}
		*cp = 0;
	    }

	    /*
	     * Look up component in current directory.
	     * Save directory inumber in case we find a
	     * symbolic link.
	     */
	    parent_inumber = inumber;
	    rc = search_directory(component, fp, &inumber);
	    if (rc) {
		goto exit;
	    }
	    *cp = c;

	    /*
	     * Open next component.
	     */
	    if ((rc = read_inode(inumber, fp)) != 0)
		goto exit;

	    /*
	     * Check for symbolic link.
	     */
	    if ((fp->i_mode & IFMT) == IFLNK) {

		int	link_len = fp->i_size;
		int	len;

		len = strlen(cp) + 1;

		if (link_len + len >= MAXPATHLEN - 1)
		    RETURN (FS_NAME_TOO_LONG);

		if (++nlinks > MAXSYMLINKS)
		    RETURN (FS_SYMLINK_LOOP);

		memmove(&namebuf[link_len], cp, len);

#ifdef	IC_FASTLINK
		if ((fp->i_flags & IC_FASTLINK) != 0) {
		    memcpy(namebuf, fp->i_symlink, (unsigned) link_len);
		}
		else
#endif	IC_FASTLINK
#if !defined(DISABLE_BSD44_FASTLINKS)
		/* 
		 * There is no bit for fastlinks in 4.4 but instead
		 * all symlinks that fit into the inode are fastlinks.
		 * If the second block (ic_db[1]) is zero the symlink
		 * can't be a fastlink if its length is at least five.
		 * For symlinks of length one to four there is no easy
		 * way of knowing whether we are looking at a 4.4
		 * fastlink or a 4.3 slowlink.  This code always
		 * guesses the 4.4 way when in doubt. THIS BREAKS 4.3
		 * SLOWLINKS OF LENGTH FOUR OR LESS.
		 */
		if ((link_len <= MAX_FASTLINK_SIZE && fp->i_ic.ic_db[1] != 0)
		     || (link_len <= 4))
		{
		    memcpy(namebuf, fp->i_symlink, (unsigned) link_len);
		}
		else
#endif        /* !DISABLE_BSD44_FASTLINKS */

		{
		    /*
		     * Read file for symbolic link
		     */
		    struct fs *fs = fp->f_fs;
		    oskit_size_t buf_size = blksize(fs, fp, 0);
		    char buf[buf_size];
		    daddr_t disk_block;
		    oskit_size_t actual;

		    rc = block_map(fp, (daddr_t)0, &disk_block);
		    if (rc)
		        goto exit;
		    rc = oskit_blkio_read(bio, buf, disk_block * DEV_BSIZE,
		    			buf_size, &actual);
		    if (rc)
			goto exit;
		    if (actual != buf_size) {
		        rc = FS_INVALID_FS;
			goto exit;
		    }

		    memcpy(namebuf, buf, (unsigned)link_len);
		}

		/*
		 * If relative pathname, restart at parent directory.
		 * If absolute pathname, restart at root.
		 * If pathname begins '/dev/<device>/',
		 *	restart at root of that device.
		 */
		cp = namebuf;
		if (*cp != '/') {
		    inumber = parent_inumber;
		}
		else {
		    inumber = (ino_t)ROOTINO;
		}
		if ((rc = read_inode(inumber, fp)) != 0)
		    goto exit;
	    }
	}

	/*
	 * Found terminal component.
	 */
	*out_fbio = &fp->f_bioi;
	return 0;

	/*
	 * At error exit, close file to free storage.
	 */
    exit:
	ffs_release(&fp->f_bioi);
	free(fp);
	return rc;
}

