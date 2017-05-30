/* -*-c-*- */
/*
 * Copyright (c) 2001 The University of Utah and the Flux Group.
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

#include <oskit/io/absio.h>
#include <oskit/io/blkio.h>
#include "oskit_uvm_internal.h"

static struct vnode swapvnode;
dev_t	swapdev = makedev(0, 0);
struct bdevsw	bdevsw[1];	/* never ref'ed */

extern void
oskit_uvm_swap_init()
{
    struct vnode *vp;

    /* from bufinit() */
    pool_init(&bufpool, sizeof(struct buf), 0, 0, 0, "bufpl", 0,
	      NULL, NULL, M_DEVBUF);

    /* Initialize swapvnode */
    vp = &swapvnode;
    bzero(vp, sizeof(*vp));
    vp->v_type = VREG;		/* XXX? */
    vp->v_mount = 0;
    vp->v_op = 0;
    vp->v_usecount = 1;
    simple_lock_init(&vp->v_interlock);
    vp->v_freelist.tqe_prev = (struct vnode **)0xdeadb;
    
    uvm_swap_init();
}

extern int
oskit_uvm_swap_on(oskit_iunknown_t *iunknown)
{
    oskit_blkio_t *pagerblkio = 0;
    oskit_absio_t *pagerabsio = 0;
    struct proc tmpproc;
    struct pcred pcred;
    struct proc *p = &tmpproc;
    struct vnode *vp;
    struct mount *mount;
    void *handle;
    struct sys_swapctl_args a;
    int retval;

    if (! oskit_iunknown_query(iunknown, &oskit_absio_iid, &handle))
	pagerabsio = (oskit_absio_t *) handle;
    else if (! oskit_iunknown_query(iunknown, &oskit_blkio_iid, &handle))
	pagerblkio = (oskit_blkio_t *) handle;
    else
	return OSKIT_EINVAL;

    /* create fake proc */
    p->p_cred = &pcred;		/* never refed! */

    mount = malloc(sizeof(*mount), M_VMSWAP, M_NOWAIT);
    vp = malloc(sizeof(*vp), M_VMSWAP, M_NOWAIT);

    bzero(vp, sizeof(*vp));
    bzero(mount, sizeof(*mount));
    vp->v_type = VREG;
    vp->v_mount = mount;
    vp->v_op = 0;
    vp->v_absio = pagerabsio;
    vp->v_blkio = pagerblkio;
    vp->v_usecount = 1;
    LIST_INIT(&vp->v_cleanblkhd);
    LIST_INIT(&vp->v_dirtyblkhd);
    simple_lock_init(&vp->v_interlock);
    vp->v_freelist.tqe_prev = (struct vnode **)0xdeadb;

    SCARG(&a, cmd) = SWAP_ON;
    SCARG(&a, arg) = vp;
    SCARG(&a, misc) = 0;	/* priority */

    return sys_swapctl(p, &a, &retval);
}


/*
 *  XXX: The swap off code in NetBSD has a bug and no interface is exported
 *  to users.  Swapping off may cause infinite loop.
 */
extern int
oskit_uvm_swap_off(oskit_iunknown_t *iunknown)
{
    oskit_blkio_t *pagerblkio = 0;
    oskit_absio_t *pagerabsio = 0;
    struct proc tmpproc;
    struct pcred pcred;
    struct proc *p = &tmpproc;
    struct vnode *vp;
    void *handle;
    struct sys_swapctl_args a;
    int retval;
    int rc;

    panic(__FUNCTION__" is broken.  Do not use!\n");

    if (! oskit_iunknown_query(iunknown, &oskit_absio_iid, &handle))
	pagerabsio = (oskit_absio_t *) handle;
    else if (! oskit_iunknown_query(iunknown, &oskit_blkio_iid, &handle))
	pagerblkio = (oskit_blkio_t *) handle;
    else
	return OSKIT_EINVAL;

    /* create fake proc */
    p->p_cred = &pcred;		/* never refed! */

    vp = malloc(sizeof(*vp), M_VMSWAP, M_NOWAIT);

    bzero(vp, sizeof(*vp));
    vp->v_absio = pagerabsio;
    vp->v_blkio = pagerblkio;

    SCARG(&a, cmd) = SWAP_OFF;
    SCARG(&a, arg) = vp;
    SCARG(&a, misc) = 0;	/* not used */

    rc = sys_swapctl(p, &a, &retval);

    free(vp, M_VMSWAP);
    if (pagerabsio) {
	oskit_absio_release(pagerabsio);
    }
    if (pagerblkio) {
	oskit_blkio_release(pagerblkio);
    }

    return rc;
}

    
static int
oskit_uvm_vop_getattr(struct vnode *vp, struct vattr *vap)
{
    oskit_off_t size;
    
    memset(vap, 0, sizeof(*vap));
    if (vp->v_absio) {
	oskit_absio_getsize(vp->v_absio, &size);
	vap->va_blocksize = PAGE_SIZE;
    } else {
	oskit_blkio_getsize(vp->v_blkio, &size);
	vap->va_blocksize = oskit_blkio_getblocksize(vp->v_blkio);
    }
    vap->va_size = (long)size;

    return 0;
}

static void
oskit_sw_reg_strategy(struct swapdev *sdp, struct buf *bp, int bn)
{
    struct vnode *vp = sdp->swd_vp;
    oskit_absio_t *aio = vp->v_absio;
    oskit_blkio_t *bio = vp->v_blkio;
    char *datap;
    size_t nbytes;
    oskit_u64_t byteoffset;
    int rc;

    byteoffset = bn * DEV_BSIZE;
    datap = (char *) bp->b_data;

    bp->b_resid = bp->b_bcount;
    do {
	UVM_UNLOCK;		/* Unlock the UVM lock while doing the I/O  */
	if (bio) {
	    oskit_u64_t offset;
	    oskit_size_t sz;

	    offset = byteoffset & ~(sdp->swd_bsize - 1);
	    /*
	     * XXXOSKIT: we have to handle offset != byteoffset situation in
	     * the future 
	     */
	    assert(offset == byteoffset);
	    sz = bp->b_bcount & ~(sdp->swd_bsize - 1);

	    pthread_mutex_lock(&sdp->swd_lock);
	    XPRINTF(OSKIT_DEBUG_SWAP, __FUNCTION__": %s addr %p, offset 0x%lx, "
		    "0x%lx bytes\n", (bp->b_flags & B_READ) ? "read" : "write",
		    datap, (long)offset, (long)sz);
	    if (bp->b_flags & B_READ) {
		rc = oskit_blkio_read(bio, datap, offset, sz, &nbytes);
	    } else {
		rc = oskit_blkio_write(bio, datap, offset, sz, &nbytes);
	    }
	    pthread_mutex_unlock(&sdp->swd_lock);
	} else {
	    pthread_mutex_lock(&sdp->swd_lock);
	    XPRINTF(OSKIT_DEBUG_SWAP, __FUNCTION__": %s addr %p, offset 0x%lx, "
		   "0x%lx bytes\n", (bp->b_flags & B_READ) ? "read" : "write",
		   datap, (long)byteoffset, (long)bp->b_bcount);
	    if (bp->b_flags & B_READ) {
		rc = oskit_absio_read(aio, datap, byteoffset,
				      bp->b_bcount, &nbytes);
	    } else {
		rc = oskit_absio_write(aio, datap, byteoffset,
				       bp->b_bcount, &nbytes);
	    }
	    pthread_mutex_unlock(&sdp->swd_lock);
	}
	UVM_LOCK;		/* Relock the UVM lock */
	
	if (rc != 0) {
		printf(__FUNCTION__": oskit_{absio,blkio}_%s() returned 0x%x\n",
		       (bp->b_flags & B_READ) ? "read" : "write", rc);
		break;
	}
	
	bp->b_resid -= nbytes;
	datap += nbytes;
	byteoffset += nbytes; 
    } while (bp->b_resid > 0);
    //splx(s);
    biodone(bp);
    return;
}

static int
oskit_uvm_vfs_statfs(vp, sbp)
    struct vnode *vp;
    struct statfs *sbp;
{
    bzero(sbp, sizeof(*sbp));
    if (vp->v_blkio) {
	sbp->f_iosize = oskit_blkio_getblocksize(vp->v_blkio);
    } else {
	sbp->f_iosize = PAGE_SIZE;	/* XXX */
    }
    return 0;
}


/****************************************************************
 *
 *	Glues
 *
 ****************************************************************/
int
bdevvp(dev_t dev, struct vnode **vpp)
{
    assert(dev == swapdev);
    *vpp = &swapvnode;
    return 0;
}

void
uvm_aio_biodone(bp)
	struct buf *bp;
{
    return;
}
	
void
brelse(bp)
	struct buf *bp;
{
    panic(__FUNCTION__);
}

void
vput(struct vnode *vp)
{
    return;
}

void
crfree(struct ucred *ucred)
{
    return;
}

struct ucred *
crdup(struct ucred *ucred)
{
    return 0;
}

void
vrele(struct vnode *vp)
{
    assert(vp->v_absio || vp->v_blkio);
    if (vp->v_absio) {
	oskit_absio_release(vp->v_absio);
    }
    if (vp->v_blkio) {
	oskit_blkio_release(vp->v_blkio);
    }
    free(vp->v_mount, M_VMSWAP);
    free(vp, M_VMSWAP);
}
