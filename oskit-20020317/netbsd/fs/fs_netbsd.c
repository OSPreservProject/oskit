/*
 * Copyright (c) 1997-2000 University of Utah and the Flux Group.
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

/*
 * Initial entrypoints into the NetBSD fs library:  
 *	fs_netbsd_init and fs_netbsd_mount
 */

#include <oskit/dev/dev.h>
#include <oskit/io/blkio.h>
#include <oskit/diskpart/diskpart.h>
#include <oskit/fs/netbsd.h>
#include "osenv.h"

#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/proc.h>

#include <oskit/c/ctype.h>

/* Allow the threaded version to override the strategy routine */
void fs_netbsd_default_wdstrategy(struct buf *bp);
void (*fs_netbsd_wdstrategy_routine)(struct buf *bp) =
		fs_netbsd_default_wdstrategy;

hashtab_t vptab;		/* vp -> oskit_file_t */	
#define VPTAB_SIZE 103

static unsigned int vphash(hashtab_key_t key)
{
    unsigned int h;
    
    h = (unsigned int) key;
    return h % VPTAB_SIZE;
}

static int vpcmp(hashtab_key_t key1, hashtab_key_t key2)
{
    return !(key1 == key2);
}

oskit_error_t fs_netbsd_init(oskit_osenv_t *osenv)
{
#ifdef INDIRECT_OSENV
    fs_netbsd_oskit_osenv_init(osenv);
#endif
    
    vptab = hashtab_create(vphash, vpcmp, VPTAB_SIZE);
    if (!vptab)
	    return OSKIT_ENOMEM;    

    /* We store the blkio address in the dev_t instead of using a table. */
    if (sizeof(dev_t) != sizeof(void *))
	    panic("dev_t and `void *' must have the same size");

    /* allow caller to override this */
    if (nbuf == 0)
	    nbuf = NBUF;
    MALLOC(buf, struct buf *, nbuf * sizeof(struct buf), M_DEVBUF, M_WAITOK);
    if (!buf)
	    return OSKIT_ENOMEM;
    MALLOC(buffers, char *, nbuf * MAXBSIZE, M_DEVBUF, M_WAITOK);
    if (!buffers)
	    return OSKIT_ENOMEM;
    bufpages = MAXBSIZE/CLBYTES * nbuf; 

    bufinit();

    vfsinit();

    return 0;
}


extern int ffs_mountroot(dev_t dev, int flags, struct mount **mp);

oskit_error_t fs_netbsd_mount(oskit_blkio_t *bio, oskit_u32_t flags,
			      oskit_filesystem_t **out_fs)
{
    struct gfilesystem *fs;
    struct mount *mp;
    struct proc *p;
    dev_t dev;
    int mflags, error; 
    oskit_error_t ferror;

    /*
     * We just stick the address of the blkio in the dev_t instead of
     * using a table.
     * We also redefined major(dev) to always return zero,
     * which is WDMAJOR.
     * That way the dev_t's are unique and we just have to implement
     * wdopen, wdstrategy, etc, which just call oskit_blkio_read, etc.
     */
    dev = (dev_t)bio;			/* ref added below */

    /* convert mount flags into internal form */
    mflags = 0;
    if (flags & OSKIT_FS_RDONLY)
	    mflags |= MNT_RDONLY;
    if (flags & OSKIT_FS_NOEXEC)
	    mflags |= MNT_NOEXEC;
    if (flags & OSKIT_FS_NOSUID)
	    mflags |= MNT_NOSUID;
    if (flags & OSKIT_FS_NODEV)
	    mflags |= MNT_NODEV;

    /*
     * For this interface, we use a modified variant of
     * the internal mountroot function, as opposed to VOP_MOUNT.
     * VOP_MOUNT expects to be provided with the paths of 
     * both the device special file and the mount point;
     * since we have neither at this point, mountroot
     * seems more apropos.  However, we do use a modified
     * variant of VOP_MOUNT for the oskit_filesystem->remount()
     * interface. 
     *
     * Our modifications to mountroot remove the assumptions
     * about using the root device number and device vnode,
     * permit immediate read-write mounts, and explicitly
     * return the newly allocated mount structure.
     */
    ferror = getproc(&p);
    if (ferror)
	    return ferror;

    /*
     * XXX need way to try different filesystems.
     * Linux just tries all registered filesystems' mount_root.
     */
    error = ffs_mountroot(dev, mflags, &mp);
    if (error)
    {
	printf("fs_netbsd_mount():  ffs_mount returned %d\n", error);
	prfree(p);
	return errno_to_oskit_error(error);
    }
    mp->mnt_stat.f_owner = p->p_ucred->cr_uid;

    /*
     * Now, create something to return.
     */
    fs = gfilesystem_create(mp);
    if (!fs)
    {
	error = dounmount(mp,MNT_FORCE,p);
	assert(error == 0);
	prfree(p);
	return OSKIT_ENOMEM;
    }
    *out_fs = &fs->fsi;

    prfree(p);

    oskit_blkio_addref(bio);

    return 0;
}


/*
 * All we have to implement are the wdopen, etc functions because we
 * defined major() to always return zero (== WDMAJOR).
 */

int wdopen(dev_t dev, int oflags, int devtype, struct proc *p)
{
    /*
     * We don't have to do anything here.
     * Our blkio ("pointed" to by dev) has already been opened
     * by the caller of fs_netbsd_mount.
     */

    return 0;
}


int wdclose(dev_t dev, int fflag, int devtype, struct proc *p)
{
    return 0;
}

void wdstrategy(struct buf *bp)
{
    fs_netbsd_wdstrategy_routine(bp);
}

void fs_netbsd_default_wdstrategy(struct buf *bp)
{
    oskit_blkio_t *bio = (oskit_blkio_t *)bp->b_dev;
    char *datap;
    size_t nbytes;
    oskit_u64_t offset;
    int s, rc;

    offset = bp->b_blkno * DEV_BSIZE;
    datap = (char *) bp->b_data;

    s = splbio();
    bp->b_resid = bp->b_bcount;
    do 
    {
	    if (bp->b_flags & B_READ) {
		SSTATE_DECL;
		
		SSTATE_SAVE;
		rc = oskit_blkio_read(bio, datap, offset,
				      bp->b_bcount, &nbytes);
		SSTATE_RESTORE;
	    }
	    else {
		SSTATE_DECL;
		
		SSTATE_SAVE;
		rc = oskit_blkio_write(bio, datap, offset,
				       bp->b_bcount, &nbytes);
		SSTATE_RESTORE;
	    }

	if (rc != 0)
	{
	    printf("wdstrategy:  oskit_blkio_%s() returned 0x%x\n",
		   (bp->b_flags & B_READ) ? "read" : "write", rc);
	    break;
	}
	
	bp->b_resid -= nbytes;
	datap += nbytes;
	offset += nbytes; 
    } while (bp->b_resid > 0);
    splx(s);

    biodone(bp);
    return;
}


int wdioctl(dev_t dev, u_long cmd, caddr_t data,
	    int fflag, struct proc *p)
{
    printf("wdioctl(cmd=0x%lx, fflag=0x%x)\n", cmd, fflag);
    return -1;
}


int wddump(dev_t dev, daddr_t blkno, caddr_t va, size_t size)
{
    panic("wddump");
}


int wdsize(dev_t dev)
{
    panic("wdsize");
}
