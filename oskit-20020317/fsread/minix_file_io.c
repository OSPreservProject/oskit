/*
 * Copyright (c) 1997-1998 University of Utah and the Flux Group.
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
 * Written by Csizmazia Balazs, University ELTE, Hungary
 */

#include <string.h>
#include <stdlib.h>	/* malloc, free */
#include <oskit/io/blkio.h>
#include <oskit/fs/read.h>


#define	MINIX_SBSIZE		MINIX_BLOCK_SIZE	/* Size of superblock */
#define	MINIX_SBLOCK		((minix_daddr_t) 2)		/* Location of superblock */

#define	MINIX_NDADDR		7
#define	MINIX_NIADDR		2

#define	MINIX_MAXNAMLEN	14

#define	MINIX_ROOTINO		1 /* MINIX ROOT INODE */

#define	MINIX_NINDIR(fs)	512 /* DISK_ADDRESSES_PER_BLOCKS */

#define	IFMT		00170000
#define	IFREG		0100000
#define	IFDIR		0040000
#define	ISVTX		0001000

typedef unsigned int	daddr_t;	/* an unsigned int */
typedef	unsigned int	ino_t;

#include "minix_fs.h"
#include "common.h"


#define	MINIX_NAME_LEN	14
#define	MINIX_BLOCK_SIZE	1024

#define	FS_NOT_DIRECTORY	OSKIT_ENOTDIR		/* not a directory */
#define	FS_NO_ENTRY		OSKIT_ENOENT		/* name not found */
#define	FS_NAME_TOO_LONG	OSKIT_ENAMETOOLONG	/* name too long */
#define	FS_SYMLINK_LOOP		OSKIT_ELOOP		/* symbolic link loop */
#define	FS_INVALID_FS		OSKIT_EINVAL		/* bad file system */
#define	FS_NOT_IN_FILE		OSKIT_EINVAL		/* offset not in file */
#define	FS_INVALID_PARAMETER	OSKIT_EINVAL		/* bad parameter to
							   a routine */

#define	MAXPATHLEN	1024

/*
 * In-core open file.
 */
struct file {
	oskit_blkio_t		f_bioi;		/* interface to this file */
	unsigned		f_refs;		/* reference count */

	oskit_blkio_t		*f_blkio;	/* underlying device handle */

	oskit_addr_t		f_buf;		/* buffer for data block */
	daddr_t			f_buf_blkno;	/* block number of data block */

	union {
		struct {
			/* pointer to super-block */
			struct minix_super_block*	minix_fs;

			/* copy of on-disk inode */
			struct minix_inode	minix_ic;

			/* number of blocks mapped by
			   indirect block at level i */
			int			minix_nindir[MINIX_NIADDR+1];

			/* buffer for indirect block at level i */
			oskit_addr_t		minix_blk[MINIX_NIADDR];

			/* size of buffer */
			oskit_size_t		minix_blksize[MINIX_NIADDR];

			/* disk address of block in buffer */
			minix_daddr_t		minix_blkno[MINIX_NIADDR];
		} minix;
	} u;
};

#define f_fs		u.minix.minix_fs
#define i_ic		u.minix.minix_ic
#define f_nindir	u.minix.minix_nindir
#define f_blk		u.minix.minix_blk
#define f_blksize	u.minix.minix_blksize
#define f_blkno		u.minix.minix_blkno


OSKIT_INLINE int
minix_ino2blk (struct minix_super_block *fs, int ino)
{
        int blk;

	blk=0 /* it's Mach */+2 /* boot+superblock */ + fs->s_imap_blocks +
		fs->s_zmap_blocks + (ino-1)/MINIX_INODES_PER_BLOCK;
        return blk;
}

OSKIT_INLINE int
minix_itoo (struct minix_super_block *fs, int ino)
{
	return (ino - 1) % MINIX_INODES_PER_BLOCK;
}

OSKIT_INLINE int
minix_blkoff (struct minix_super_block * fs, oskit_addr_t offset)
{
	return offset % MINIX_BLOCK_SIZE;
}

OSKIT_INLINE int
minix_lblkno (struct minix_super_block * fs, oskit_addr_t offset)
{
	return offset / MINIX_BLOCK_SIZE;
}

OSKIT_INLINE int
minix_blksize (struct minix_super_block *fs, struct file *fp, minix_daddr_t file_block)
{
	return MINIX_BLOCK_SIZE;
}

/*
 * Free file buffers, but don't close file.
 */
static void
free_file_buffers(fp)
	struct file	*fp;
{
	int level;

	/*
	 * Free the indirect blocks
	 */
	for (level = 0; level < MINIX_NIADDR; level++) {
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
	struct file	*fp;
{
	struct minix_super_block	*fs = fp->f_fs;
	oskit_size_t		buf_size = MINIX_BLOCK_SIZE;
	char			buf[buf_size];
	minix_daddr_t		disk_block;
	oskit_size_t		actual;
	oskit_error_t		rc;

	disk_block = minix_ino2blk(fs, inumber);

	rc = oskit_blkio_read(fp->f_blkio, buf,
		(oskit_off_t)disk_block * buf_size, buf_size, &actual);
	if (rc != 0)
		return rc;
	if (actual != buf_size)
		return OSKIT_E_UNEXPECTED; /* XXX bad FS format */

	{
	    struct minix_inode *dp;

	    dp = (struct minix_inode *)buf;
	    dp += minix_itoo(fs, inumber);
	    fp->i_ic = *dp;
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
	minix_daddr_t	file_block;
	minix_daddr_t	*disk_block_p;	/* out */
{
	int		level;
	int		idx;
	minix_daddr_t	ind_block_num;
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

	if (file_block < MINIX_NDADDR) {
	    /* Direct block. */
	    *disk_block_p = fp->i_ic.i_zone[file_block];
	    return (0);
	}

	file_block -= MINIX_NDADDR;

	/*
	 * nindir[0] = NINDIR
	 * nindir[1] = NINDIR**2
	 * nindir[2] = NINDIR**3
	 *	etc
	 */
	for (level = 0; level < MINIX_NIADDR; level++) {
	    if (file_block < fp->f_nindir[level])
		break;
	    file_block -= fp->f_nindir[level];
	}
	if (level == MINIX_NIADDR) {
	    /* Block number too high */
	    return (FS_NOT_IN_FILE);
	}

	ind_block_num = fp->i_ic.i_zone[level + MINIX_NDADDR];

	for (; level >= 0; level--) {

	    oskit_size_t	size = MINIX_BLOCK_SIZE;
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
			(oskit_off_t)ind_block_num * size, size, &actual);
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

	    ind_block_num = ((minix_daddr_t *)fp->f_blk[level])[idx];
	}

	*disk_block_p = ind_block_num;
	return (0);
}

/*
 * Read a portion of a file into an internal buffer.  Return
 * the location in the buffer and the amount in the buffer.
 */
static int
buf_read_file(fp, offset, buf_p, size_p)
	struct file		*fp;
	oskit_addr_t		offset;
	oskit_addr_t		*buf_p;		/* out */
	oskit_size_t		*size_p;	/* out */
{
	struct minix_super_block	*fs;
	oskit_addr_t		off;
	minix_daddr_t		file_block;
	minix_daddr_t		disk_block;
	int			rc;
	oskit_addr_t		block_size;

	if (offset >= fp->i_ic.i_size) {
	    *size_p = 0;
	    return 0;
	}

	fs = fp->f_fs;

	off = minix_blkoff(fs, offset);
	file_block = minix_lblkno(fs, offset);
	block_size = minix_blksize(fs, fp, file_block);

	if (((daddr_t) file_block) != fp->f_buf_blkno) {
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
					    (oskit_off_t)disk_block * block_size,
					    block_size, &actual);
		if (rc)
			return rc;
		if (actual != block_size)
			return OSKIT_E_UNEXPECTED;
	    }

	    /* Remember which block this is */
	    fp->f_buf_blkno = (daddr_t) file_block;
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
	if (*size_p > fp->i_ic.i_size - offset)
	    *size_p = fp->i_ic.i_size - offset;

	return (0);
}

/*
 * Search a directory for a name and return its
 * i_number.
 */
static int
search_directory(name, fp, inumber_p)
	char *		name;
	struct file	*fp;
	ino_t		*inumber_p;	/* out */
{
	oskit_addr_t	buf;
	oskit_size_t	buf_size;
	oskit_addr_t	offset;
	struct minix_directory_entry *dp;
	int		length;
	oskit_error_t	rc;
	char		tmp_name[15];

	length = strlen(name);

	offset = 0;
	while (offset < fp->i_ic.i_size) {
	    rc = buf_read_file(fp, offset, &buf, &buf_size);
	    if (rc != 0)
		return (rc);
	    if (buf_size == 0)
		break;

	    dp = (struct minix_directory_entry *)buf;
	    if (dp->inode != 0) {
		strncpy (tmp_name, dp->name, MINIX_NAME_LEN /* XXX it's 14 */);
		tmp_name[MINIX_NAME_LEN] = '\0';
		if (strlen(tmp_name) == length &&
		    !strcmp(name, tmp_name))
	    	{
		    /* found entry */
		    *inumber_p = dp->inode;
		    return (0);
		}
	    }
	    offset += 16 /* MINIX dir. entry length - MINIX FS Ver. 1. */;
	}
	return (FS_NO_ENTRY);
}

static int
read_fs(bio, fsp)
	oskit_blkio_t		*bio;
	struct minix_super_block **fsp;
{
	struct minix_super_block *fs;
	oskit_addr_t		actual;
	int			error;

	/*
	 * Read the super block
	 */
	fs = malloc(MINIX_SBSIZE);
	if (fs == NULL)
		return OSKIT_E_OUTOFMEMORY;
	error = oskit_blkio_read(bio, fs, MINIX_SBLOCK * MINIX_SBSIZE,
				MINIX_SBSIZE, &actual);
	if (error) {
		free(fs);
		return (error);
	if (actual != MINIX_SBSIZE)
		free(fs);
		return FS_INVALID_FS;
	}

	/*
	 * Check the superblock
	 */
	if (fs->s_magic != MINIX_SUPER_MAGIC) {
		free(fs);
		return (FS_INVALID_FS);
	}

	*fsp = fs;
	return 0;
}

static int
mount_fs(fp)
	struct file	*fp;
{
	struct minix_super_block *fs;
	int error;

	error = read_fs(fp->f_blkio, &fp->f_fs);
	if (error)
	    return (error);

	fs = fp->f_fs;

	/*
	 * Calculate indirect block levels.
	 */
	{
	    int	mult;
	    int	level;

	    mult = 1;
	    for (level = 0; level < MINIX_NIADDR; level++) {
		mult *= MINIX_NINDIR(fs);
		fp->f_nindir[level] = mult;
	    }
	}

	return (0);
}

static void
unmount_fs(fp)
	struct file	*fp;
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
minix_release(oskit_blkio_t *io)
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
minix_read(oskit_blkio_t *io, void *start, oskit_off_t offset, oskit_size_t size,
	  oskit_size_t *out_actual)
{
	struct file		*fp = (struct file*)io;
	int			rc;
	oskit_size_t	csize;
	oskit_addr_t		buf;
	oskit_size_t		buf_size;
	oskit_size_t		actual = 0;

	while (size != 0) {
	    rc = buf_read_file(fp, offset, &buf, &buf_size);
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
minix_getsize(oskit_blkio_t *io, oskit_off_t *out_size)
{
	struct file *fp = (struct file*)io;

	*out_size = fp->i_ic.i_size;
	return 0;
}

static struct oskit_blkio_ops biops = {
	fsread_common_query,
	fsread_common_addref,
	minix_release,
	fsread_common_getblocksize,
	minix_read,
	fsread_common_write,
	minix_getsize,
	fsread_common_setsize
};


/*
 * Open a file.
 */
oskit_error_t
fsread_minix_open(oskit_blkio_t *bio, const char *path, oskit_blkio_t **out_fbio)
{
#define	RETURN(code)	{ rc = (code); goto exit; }

	char	*cp, *component;
	int	c;	/* char */
	int	rc;
	ino_t		inumber, parent_inumber;
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

	inumber = (ino_t) MINIX_ROOTINO;
	if ((rc = read_inode(inumber, fp)) != 0) {
	    goto exit;
	}

	while (*cp) {

	    /*
	     * Check that current node is a directory.
	     */
	    if ((fp->i_ic.i_mode & IFMT) != IFDIR)
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
		    if (len++ > MINIX_MAXNAMLEN)
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
	minix_release(&fp->f_bioi);
	return rc;
}

