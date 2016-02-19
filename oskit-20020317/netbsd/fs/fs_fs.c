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

#include <oskit/io/blkio.h>

#include "fs_glue.h"
#undef TRUE
#undef FALSE
#include <vm/vm.h>
#include <ufs/ufs/quota.h>
#include <ufs/ufs/ufsmount.h>

/*
 * oskit_filesystem internal constructor
 */

static struct oskit_filesystem_ops fs_ops;

struct gfilesystem *gfilesystem_create(struct mount *mp) 
{
    struct gfilesystem *fs;
    
    MALLOC(fs,struct gfilesystem*,sizeof(struct gfilesystem),M_TEMP,M_WAITOK);
    if (!fs)
	    return NULL;

    fs->fsi.ops = &fs_ops; 
    fs->count = 1;
    fs->mp = mp; 

    return fs;
}


/*
 * oskit_filesystem methods
 */

static OSKIT_COMDECL filesystem_query(oskit_filesystem_t *f,
				     const struct oskit_guid *iid,
				     void **out_ihandle)
{
    struct gfilesystem *fs;

    fs = (struct gfilesystem*)f;
    if (fs == NULL)
	    panic("%s:%d: null filesystem", __FILE__, __LINE__);
    if (fs->count == 0)
	    panic("%s:%d: bad count", __FILE__, __LINE__);
    
    if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
	memcmp(iid, &oskit_filesystem_iid, sizeof(*iid)) == 0) {
	*out_ihandle = &fs->fsi;
	++fs->count;
	return 0;
    }

    *out_ihandle = NULL;
    return OSKIT_E_NOINTERFACE;    
}


static OSKIT_COMDECL_U filesystem_addref(oskit_filesystem_t *f)
{
    struct gfilesystem *fs;

    fs = (struct gfilesystem*)f;
    if (fs == NULL)
	    panic("%s:%d: null filesystem", __FILE__, __LINE__);
    if (fs->count == 0)
	    panic("%s:%d: bad count", __FILE__, __LINE__);

    return ++fs->count;
}


static OSKIT_COMDECL_U filesystem_release(oskit_filesystem_t *f)
{
    struct gfilesystem *fs;
    struct proc *p;
    dev_t dev;
    int rc;
    unsigned newcount;
    
    fs = (struct gfilesystem*)f; 
    if (fs == NULL)
	    panic("%s:%d: null filesystem", __FILE__, __LINE__);
    if (fs->count == 0)
	    panic("%s:%d: bad count", __FILE__, __LINE__);    

    newcount = --fs->count;
    if (newcount == 0)
    {
	rc = getproc(&p);
	assert(rc == 0);
	if (fs->mp)
	{
	    dev = ((struct ufsmount *)fs->mp->mnt_data)->um_dev;
	    rc = dounmount(fs->mp, MNT_FORCE, p);
	    assert(rc == 0);
	    oskit_blkio_release((oskit_blkio_t *)dev);
	}
	prfree(p);
	free(fs,M_TEMP);
    }

    return newcount;
}


static OSKIT_COMDECL filesystem_statfs(oskit_filesystem_t *f,
				      oskit_statfs_t *out_stats)
{
    struct gfilesystem *fs = (struct gfilesystem *) f; 
    struct statfs *sp;
    struct mount *mp;
    struct proc *p;
    oskit_error_t ferror;
    int error;

    
    if (!fs || !fs->count || !fs->mp)
	    return OSKIT_E_INVALIDARG;

    ferror = getproc(&p);
    if (ferror)
	    return ferror;
    
    mp = fs->mp;
    sp = &mp->mnt_stat;
    error = VFS_STATFS(fs->mp, sp, p);
    prfree(p);
    if (error)
	    return errno_to_oskit_error(error);

    sp->f_flags = mp->mnt_flag & MNT_VISFLAGMASK;    

    out_stats->flag = 0;
    if (sp->f_flags & MNT_RDONLY)
	    out_stats->flag |= OSKIT_FS_RDONLY;
    if (sp->f_flags & MNT_NOEXEC)
	    out_stats->flag |= OSKIT_FS_NOEXEC;
    if (sp->f_flags & MNT_NOSUID)
	    out_stats->flag |= OSKIT_FS_NOSUID;
    if (sp->f_flags & MNT_NODEV)
	    out_stats->flag |= OSKIT_FS_NODEV;
    
    out_stats->bsize = sp->f_bsize;
    out_stats->frsize = sp->f_bsize;
    out_stats->blocks = sp->f_blocks;
    out_stats->bfree = sp->f_bfree;
    out_stats->bavail = sp->f_bavail;
    out_stats->files = sp->f_files;
    out_stats->ffree = sp->f_ffree;
    out_stats->favail = sp->f_ffree;
    out_stats->fsid = sp->f_fsid.val[0]; /* device number */
    out_stats->namemax = NAME_MAX;
    
    return 0;
}


static OSKIT_COMDECL filesystem_sync(oskit_filesystem_t *f,
				    oskit_bool_t wait)
{
    struct gfilesystem *fs = (struct gfilesystem *) f; 
    struct mount *mp;
    struct proc *p;
    oskit_error_t ferror;
    int error, asyncflag;

    
    if (!fs || !fs->count || !fs->mp)
	    return OSKIT_E_INVALIDARG;

    ferror = getproc(&p);
    if (ferror)
	    return ferror;

    mp = fs->mp;
    error = 0;
    if ((mp->mnt_flag & (MNT_MLOCK|MNT_RDONLY|MNT_MPBUSY)) == 0 &&
	!vfs_busy(mp)) {
	asyncflag = mp->mnt_flag & MNT_ASYNC;
	mp->mnt_flag &= ~MNT_ASYNC;
	error = VFS_SYNC(mp, wait ? MNT_WAIT : MNT_NOWAIT, p->p_ucred, p);
	if (asyncflag)
		mp->mnt_flag |= MNT_ASYNC;
	vfs_unbusy(mp);
    }
    prfree(p);
    if (error)
	    return errno_to_oskit_error(error);	

    return 0;
}


static OSKIT_COMDECL filesystem_getroot(oskit_filesystem_t *f,
				       struct oskit_dir **out_dir)
{
    struct gfilesystem *fs = (struct gfilesystem *) f; 
    oskit_dir_t *d;
    struct proc *p;
    oskit_error_t ferror;
    struct vnode *vp;
    int error;

    if (!fs || !fs->count || !fs->mp)
	    return OSKIT_E_INVALIDARG;

    ferror = getproc(&p);
    if (ferror)
	    return ferror;
    
    error = VFS_ROOT(fs->mp, &vp);
    if (error)
    {
	prfree(p);
	return errno_to_oskit_error(error);
    }
    
    error = VOP_ACCESS(vp, VEXEC, p->p_ucred, p);
    if (error)
    {
	vput(vp);
	prfree(p);
	return errno_to_oskit_error(error);
    }
    
    d = (oskit_dir_t *) hashtab_search(vptab, (hashtab_key_t) vp);
    if (d)
    {
	oskit_dir_addref(d);
    }
    else
    {
	d = (oskit_dir_t *) gfile_create(fs,vp);
	if (!d)
	{
	    vput(vp);
	    prfree(p);
	    return OSKIT_ENOMEM;
	}
    }

    vput(vp);
    prfree(p);
    *out_dir = d;
    return 0;    
}


static OSKIT_COMDECL filesystem_remount(oskit_filesystem_t *f,
				       oskit_u32_t flags)
{
    struct gfilesystem *fs = (struct gfilesystem *) f; 
    struct mount *mp;
    struct proc *p;
    oskit_error_t ferror;
    int error, oflag;

    
    if (!fs || !fs->count || !fs->mp)
	    return OSKIT_E_INVALIDARG;

    ferror = getproc(&p);
    if (ferror)
	    return ferror;

    mp = fs->mp;

    /*
     * Only root, or the user that did the original mount is
     * permitted to update it.
     */
    if (mp->mnt_stat.f_owner != p->p_ucred->cr_uid &&
	(error = suser(p->p_ucred, 0))) 
    {
	prfree(p);
	return errno_to_oskit_error(error);
    }

    if (flags & OSKIT_FS_NOEXEC)
	    mp->mnt_flag |= MNT_NOEXEC;
    if (flags & OSKIT_FS_NOSUID)
	    mp->mnt_flag |= MNT_NOSUID;
    if (flags & OSKIT_FS_NODEV)
	    mp->mnt_flag |= MNT_NODEV;

    oflag = mp->mnt_flag;

    if ((flags & OSKIT_FS_RDONLY) &&
	((mp->mnt_flag & MNT_RDONLY) == 0))
	    /* rdwr -> rdonly */
	    mp->mnt_flag |= (MNT_UPDATE | MNT_FORCE | MNT_RDONLY);
    
    if (((flags & OSKIT_FS_RDONLY) == 0) &&
	(mp->mnt_flag & MNT_RDONLY))
	    /* rdonly -> rdwr */
	    mp->mnt_flag |= (MNT_UPDATE | MNT_RELOAD | MNT_WANTRDWR);

    error = 0;
    if (mp->mnt_flag & MNT_UPDATE)
    {
	struct nameidata nd;		/* dummy to contain cred */
	struct ucred cred;		/* dummy, contents not actually used */

	nd.ni_cnd.cn_cred = &cred;
	error = VFS_MOUNT(mp, 0, 0, &nd, p);
	if (mp->mnt_flag & MNT_WANTRDWR)
		mp->mnt_flag &= ~MNT_RDONLY;
	mp->mnt_flag &=~ (MNT_UPDATE | MNT_RELOAD | MNT_FORCE | MNT_WANTRDWR);
	if (error)
		mp->mnt_flag = oflag;
    }

    prfree(p);
    if (error)
	    return errno_to_oskit_error(error);	

    return 0;    
}


static OSKIT_COMDECL filesystem_unmount(oskit_filesystem_t *f)
{
    struct gfilesystem *fs = (struct gfilesystem *) f; 
    struct mount *mp;
    dev_t dev;
    struct proc *p;
    oskit_error_t ferror;
    int error;

    
    if (!fs || !fs->count || !fs->mp)
	    return OSKIT_E_INVALIDARG;

    ferror = getproc(&p);
    if (ferror)
	    return ferror;

    mp = fs->mp;

    /*
     * Only root, or the user that did the original mount is
     * permitted to forcibly unmount it.
     */
    if (mp->mnt_stat.f_owner != p->p_ucred->cr_uid &&
	(error = suser(p->p_ucred, 0))) 
    {
	prfree(p);
	return errno_to_oskit_error(error);
    }

    /* Get the blkio "pointed" to by the dev_t so we can release it below. */
    dev = ((struct ufsmount *)mp->mnt_data)->um_dev;

    error = dounmount(fs->mp, MNT_FORCE, p);
    fs->mp = 0;
    prfree(p);
    oskit_blkio_release((oskit_blkio_t *)dev);
    if (error)
	    return errno_to_oskit_error(error);	

    return 0;    
}

static OSKIT_COMDECL
filesystem_lookupi(oskit_filesystem_t *f, oskit_ino_t ino,
		   oskit_file_t **out_file)
{
	return OSKIT_E_NOTIMPL;
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

    

