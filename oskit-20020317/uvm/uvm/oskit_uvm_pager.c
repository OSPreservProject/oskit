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

/* 
 * OSKit absio/blkio pager.  All I/Os are synchronous for now.
 * Code needs to be cleaned up.  This code is derived from mostly 
 * uvm_vnode.c and bit from genfs_vnops.c.
 */

/*	$NetBSD: uvm_vnode.c,v 1.39 2000/12/06 03:37:30 chs Exp $	*/

/*
 * Copyright (c) 1997 Charles D. Cranor and Washington University.
 * Copyright (c) 1991, 1993
 *      The Regents of the University of California.  
 * Copyright (c) 1990 University of Utah.
 *
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
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
 *      This product includes software developed by Charles D. Cranor,
 *	Washington University, the University of California, Berkeley and 
 *	its contributors.
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
 *
 *      @(#)vnode_pager.c       8.8 (Berkeley) 2/13/94
 * from: Id: uvm_vnode.c,v 1.1.2.26 1998/02/02 20:38:07 chuck Exp
 */

#include <sys/param.h>
#include <sys/malloc.h>
#include <uvm/uvm.h>
#include <oskit/io/blkio.h>
#include <oskit/io/absio.h>
#include "oskit_uvm_internal.h"

extern boolean_t uvn_releasepg(struct vm_page *pg, struct vm_page **nextpgp);

struct uvm_blkio_node {
    struct uvm_object		u_obj;

    /* only one of pagerabsio and pagerblkio can be used */
    oskit_absio_t		*pagerabsio;
    oskit_blkio_t		*pagerblkio;
    oskit_off_t			size;
    int				refcount;

    LIST_ENTRY(uvm_blkio_node)	list;
};

LIST_HEAD(blkio_head, uvm_blkio_node) blkio_list;

/*
 * functions
 */

static void		blkio_detach __P((struct uvm_object *));
static boolean_t	blkio_flush __P((struct uvm_object *, voff_t, voff_t,
				       int));
static int		blkio_get __P((struct uvm_object *, voff_t, vm_page_t *,
				     int *, int, vm_prot_t, int, int));
static int		blkio_put __P((struct uvm_object *, vm_page_t *, int,
				     boolean_t));
static void		blkio_reference __P((struct uvm_object *));
static boolean_t	blkio_releasepg __P((struct vm_page *,
					   struct vm_page **));
/*
 * master pager structure
 */

/*struct uvm_pagerops uvm_blkio_pager = {*/
struct uvm_pagerops uvm_vnodeops = {
	NULL,
	blkio_reference,
	blkio_detach,
	NULL,
	blkio_flush,
	blkio_get,
	blkio_put,
	NULL,
	NULL,
	blkio_releasepg,
};

/*
 * the ops!
 */

/*
 * blkio_attach
 *
 * attach a vnode structure to a VM object.  if the vnode is already
 * attached, then just bump the reference count by one and return the
 * VM object.   if not already attached, attach and return the new VM obj.
 * the "accessprot" tells the max access the attaching thread wants to
 * our pages.
 *
 * => caller must _not_ already be holding the lock on the uvm_object.
 * => in fact, nothing should be locked so that we can sleep here.
 * => note that uvm_object is first thing in vnode structure, so their
 *    pointers are equiv.
 */

struct uvm_object *
oskit_blkio_attach(oskit_iunknown_t *iunknown)
{
    struct uvm_blkio_node *bobj, *node;
    oskit_off_t size;
    oskit_blkio_t *pagerblkio = 0;
    oskit_absio_t *pagerabsio = 0;
    void *handle;

    /*
     * Make sure we got an absio or a blkio. We add a reference.
     */
    if (! oskit_iunknown_query(iunknown, &oskit_absio_iid, &handle))
	pagerabsio = (oskit_absio_t *) handle;
    else if (! oskit_iunknown_query(iunknown, &oskit_blkio_iid, &handle))
	pagerblkio = (oskit_blkio_t *) handle;
    else
	return NULL;

    LIST_FOREACH(node, &blkio_list, list) {
	if (node->pagerabsio == pagerabsio
	    && node->pagerblkio == pagerblkio) {
	    node->refcount++;
	    return &node->u_obj;
	}
    }

    /* allocate uvm_blkio_node structure */
    bobj = malloc(sizeof(*bobj), M_TEMP, M_WAITOK);
    simple_lock_init(&bobj->u_obj.vmobjlock);
    bobj->u_obj.pgops = &uvm_vnodeops;
    TAILQ_INIT(&bobj->u_obj.memq);
    bobj->u_obj.uo_npages = 0;

    bobj->pagerabsio = pagerabsio;
    bobj->pagerblkio = pagerblkio;

    if (pagerabsio) {
	oskit_absio_getsize(pagerabsio, &size);
    } else {
	oskit_blkio_getsize(pagerblkio, &size);
    }

    bobj->size = size;
    bobj->u_obj.uo_refs = 1;	/* start with 1 reference */
    bobj->refcount = 1;

    LIST_INSERT_HEAD(&blkio_list, bobj, list);

    return &bobj->u_obj;
}


/*
 * blkio_reference
 *
 * duplicate a reference to a VM object.  Note that the reference
 * count must already be at least one (the passed in reference) so 
 * there is no chance of the uvn being killed or locked out here.
 *
 * => caller must call with object unlocked.  
 * => caller must be using the same accessprot as was used at attach time
 */


static void
blkio_reference(uobj)
	struct uvm_object *uobj;
{
    struct uvm_blkio_node *bobj = (struct uvm_blkio_node *)uobj;

    simple_lock(&uobj->vmobjlock);
    bobj->refcount++;
    simple_unlock(&uobj->vmobjlock);
}

/*
 * blkio_detach
 *
 * remove a reference to a VM object.
 *
 * => caller must call with object unlocked and map locked.
 * => this starts the detach process, but doesn't have to finish it
 *    (async i/o could still be pending).
 */
static void
blkio_detach(uobj)
	struct uvm_object *uobj;
{
    struct uvm_blkio_node *bobj = (struct uvm_blkio_node *)uobj;
    boolean_t busybody;
    struct vm_page *pg;

    simple_lock(&uobj->vmobjlock);
    bobj->refcount--;

    if (bobj->refcount > 0) {
	simple_unlock(&uobj->vmobjlock);
	return;
    } 

#if 0
    /*
     * perform outstanding I/Os.  This ensures dirty pages write back
     * to the backing store.  XXX
     */
    blkio_flush(uobj, 0, 0, PGO_ALLPAGES|PGO_CLEANIT);
#endif
    
    /*
     * free all the pages that aren't PG_BUSY,
     * mark for release any that are.
     */
    busybody = FALSE;
    for (pg = TAILQ_FIRST(&uobj->memq);
	 pg != NULL;
	 pg = TAILQ_NEXT(pg, listq)) {
	if (pg->flags & PG_BUSY) {
	    pg->flags |= PG_RELEASED;
	    busybody = TRUE;
	    continue;
	}
	
	/* zap the mappings, free the page */
	pmap_page_protect(pg, VM_PROT_NONE);
	uvm_lock_pageq();
	uvm_pagefree(pg);
	uvm_unlock_pageq();
    }

    /*
     * if we found any busy pages, we're done for now.
     * mark the bobj for death, releasepg will finish up for us.
     */
    if (busybody) {
#if 0
	bobj->u_flags |= UAO_FLAG_KILLME;
#endif
	printf(__FUNCTION__": some pages are busy\n");
	simple_unlock(&bobj->u_obj.vmobjlock);
	return;
    }

    if (bobj->pagerabsio) {
	oskit_absio_release(bobj->pagerabsio);
    } else {
	oskit_blkio_release(bobj->pagerblkio);
    }
    
    LIST_REMOVE(bobj, list);
    simple_unlock(&uobj->vmobjlock);
    free(bobj, M_TEMP);
}

/*
 * blkio_releasepg: handled a released page in a uvn
 *
 * => "pg" is a PG_BUSY [caller owns it], PG_RELEASED page that we need
 *	to dispose of.
 * => caller must handled PG_WANTED case
 * => called with page's object locked, pageq's unlocked
 * => returns TRUE if page's object is still alive, FALSE if we
 *	killed the page's object.    if we return TRUE, then we
 *	return with the object locked.
 * => if (nextpgp != NULL) => we return the next page on the queue, and return
 *				with the page queues locked [for pagedaemon]
 * => if (nextpgp == NULL) => we return with page queues unlocked [normal case]
 * => we kill the uvn if it is not referenced and we are suppose to
 *	kill it ("relkill").
 */

boolean_t
blkio_releasepg(pg, nextpgp)
	struct vm_page *pg;
	struct vm_page **nextpgp;	/* OUT */
{
	KASSERT(pg->flags & PG_RELEASED);
	
	/*
	 * dispose of the page [caller handles PG_WANTED]
	 */
	pmap_page_protect(pg, VM_PROT_NONE);
	uvm_lock_pageq();
	if (nextpgp)
		*nextpgp = TAILQ_NEXT(pg, pageq);
	uvm_pagefree(pg);
	if (!nextpgp)
		uvm_unlock_pageq();

	return (TRUE);
}

/*
 * NOTE: currently we have to use VOP_READ/VOP_WRITE because they go
 * through the buffer cache and allow I/O in any size.  These VOPs use
 * synchronous i/o.  [vs. VOP_STRATEGY which can be async, but doesn't
 * go through the buffer cache or allow I/O sizes larger than a
 * block].  we will eventually want to change this.
 *
 * issues to consider:
 *   uvm provides the uvm_aiodesc structure for async i/o management.
 * there are two tailq's in the uvm. structure... one for pending async
 * i/o and one for "done" async i/o.   to do an async i/o one puts
 * an aiodesc on the "pending" list (protected by splbio()), starts the
 * i/o and returns VM_PAGER_PEND.    when the i/o is done, we expect
 * some sort of "i/o done" function to be called (at splbio(), interrupt
 * time).   this function should remove the aiodesc from the pending list
 * and place it on the "done" list and wakeup the daemon.   the daemon
 * will run at normal spl() and will remove all items from the "done"
 * list and call the "aiodone" hook for each done request (see uvm_pager.c).
 * [in the old vm code, this was done by calling the "put" routine with
 * null arguments which made the code harder to read and understand because
 * you had one function ("put") doing two things.]  
 *
 * so the current pager needs: 
 *   int blkio_aiodone(struct uvm_aiodesc *)
 *
 * => return KERN_SUCCESS (aio finished, free it).  otherwise requeue for
 *	later collection.
 * => called with pageq's locked by the daemon.
 *
 * general outline:
 * - "try" to lock object.   if fail, just return (will try again later)
 * - drop "u_nio" (this req is done!)
 * - if (object->iosync && u_naio == 0) { wakeup &uvn->u_naio }
 * - get "page" structures (atop?).
 * - handle "wanted" pages
 * - handle "released" pages [using pgo_releasepg]
 *   >>> pgo_releasepg may kill the object
 * dont forget to look at "object" wanted flag in all cases.
 */


/*
 * blkio_flush: flush pages out of a uvm object.
 *
 * => object should be locked by caller.   we may _unlock_ the object
 *	if (and only if) we need to clean a page (PGO_CLEANIT).
 *	we return with the object locked.
 * => if PGO_CLEANIT is set, we may block (due to I/O).   thus, a caller
 *	might want to unlock higher level resources (e.g. vm_map)
 *	before calling flush.
 * => if PGO_CLEANIT is not set, then we will neither unlock the object
 *	or block.
 * => if PGO_ALLPAGE is set, then all pages in the object are valid targets
 *	for flushing.
 * => NOTE: we rely on the fact that the object's memq is a TAILQ and
 *	that new pages are inserted on the tail end of the list.   thus,
 *	we can make a complete pass through the object in one go by starting
 *	at the head and working towards the tail (new pages are put in
 *	front of us).
 * => NOTE: we are allowed to lock the page queues, so the caller
 *	must not be holding the lock on them [e.g. pagedaemon had
 *	better not call us with the queues locked]
 * => we return TRUE unless we encountered some sort of I/O error
 *
 * comment on "cleaning" object and PG_BUSY pages:
 *	this routine is holding the lock on the object.   the only time
 *	that it can run into a PG_BUSY page that it does not own is if
 *	some other process has started I/O on the page (e.g. either
 *	a pagein, or a pageout).    if the PG_BUSY page is being paged
 *	in, then it can not be dirty (!PG_CLEAN) because no one has
 *	had a chance to modify it yet.    if the PG_BUSY page is being
 *	paged out then it means that someone else has already started
 *	cleaning the page for us (how nice!).    in this case, if we 
 *	have syncio specified, then after we make our pass through the
 *	object we need to wait for the other PG_BUSY pages to clear 
 *	off (i.e. we need to do an iosync).   also note that once a
 *	page is PG_BUSY it must stay in its object until it is un-busyed.
 *
 * note on page traversal:
 *	we can traverse the pages in an object either by going down the
 *	linked list in "uobj->memq", or we can go over the address range
 *	by page doing hash table lookups for each address.    depending
 *	on how many pages are in the object it may be cheaper to do one 
 *	or the other.   we set "by_list" to true if we are using memq.
 *	if the cost of a hash lookup was equal to the cost of the list
 *	traversal we could compare the number of pages in the start->stop
 *	range to the total number of pages in the object.   however, it
 *	seems that a hash table lookup is more expensive than the linked
 *	list traversal, so we multiply the number of pages in the 
 *	start->stop range by a penalty which we define below.
 */

#define UVN_HASH_PENALTY 4	/* XXX: a guess */

static boolean_t
blkio_flush(uobj, start, stop, flags)
	struct uvm_object *uobj;
	voff_t start, stop;
	int flags;
{
	struct uvm_blkio_node *bobj = (struct uvm_blkio_node *)uobj;
	struct vm_page *pp, *ppnext, *ptmp;
	struct vm_page *pps[256], **ppsp;
	int npages, result, lcv;
	boolean_t retval, need_iosync, by_list, needs_clean, all, wasclean;
	voff_t curoff;
	u_short pp_version;
	UVMHIST_FUNC("blkio_flush"); UVMHIST_CALLED(maphist);
	UVMHIST_LOG(maphist, "uobj %p start 0x%x stop 0x%x flags 0x%x",
		    uobj, start, stop, flags);
	KASSERT(flags & (PGO_CLEANIT|PGO_FREE|PGO_DEACTIVATE));

#if 0
#ifdef DEBUG
	if (bobj->u_size == VSIZENOTSET) {
		printf("blkio_flush: size not set vp %p\n", uvn);
		vprint("blkio_flush VSIZENOTSET", vp);
		flags |= PGO_ALLPAGES;
	}
#endif
#endif

	/*
	 * get init vals and determine how we are going to traverse object
	 */

	curoff = 0;
	need_iosync = FALSE;
	retval = TRUE;
	wasclean = TRUE;
	if (flags & PGO_ALLPAGES) {
		all = TRUE;
		by_list = TRUE;
	} else {
		start = trunc_page(start);
		stop = round_page(stop);
#ifdef DEBUG
		if (stop > round_page(bobj->size)) {
			printf("blkio_flush: oor bobj %p start 0x%x stop 0x%x "
			       "size 0x%x\n", bobj, (int)start, (int)stop,
			       (int)round_page(bobj->size));
		}
#endif
		all = FALSE;
		by_list = (uobj->uo_npages <= 
		    ((stop - start) >> PAGE_SHIFT) * UVN_HASH_PENALTY);
	}

	UVMHIST_LOG(maphist,
	    " flush start=0x%x, stop=0x%x, by_list=%d, flags=0x%x",
	    start, stop, by_list, flags);

	/*
	 * PG_CLEANCHK: this bit is used by the pgo_mk_pcluster function as
	 * a _hint_ as to how up to date the PG_CLEAN bit is.   if the hint
	 * is wrong it will only prevent us from clustering... it won't break
	 * anything.   we clear all PG_CLEANCHK bits here, and pgo_mk_pcluster
	 * will set them as it syncs PG_CLEAN.   This is only an issue if we
	 * are looking at non-inactive pages (because inactive page's PG_CLEAN
	 * bit is always up to date since there are no mappings).
	 * [borrowed PG_CLEANCHK idea from FreeBSD VM]
	 */

	if ((flags & PGO_CLEANIT) != 0 &&
	    uobj->pgops->pgo_mk_pcluster != NULL) {
		if (by_list) {
			TAILQ_FOREACH(pp, &uobj->memq, listq) {
				if (!all &&
				    (pp->offset < start || pp->offset >= stop))
					continue;
				pp->flags &= ~PG_CLEANCHK;
			}

		} else {   /* by hash */
			for (curoff = start ; curoff < stop;
			    curoff += PAGE_SIZE) {
				pp = uvm_pagelookup(uobj, curoff);
				if (pp)
					pp->flags &= ~PG_CLEANCHK;
			}
		}
	}

	/*
	 * now do it.   note: we must update ppnext in body of loop or we
	 * will get stuck.  we need to use ppnext because we may free "pp"
	 * before doing the next loop.
	 */

	if (by_list) {
		pp = TAILQ_FIRST(&uobj->memq);
	} else {
		curoff = start;
		pp = uvm_pagelookup(uobj, curoff);
	}

	ppnext = NULL;
	ppsp = NULL;
	uvm_lock_pageq();

	/* locked: both page queues and uobj */
	for ( ; (by_list && pp != NULL) || 
		      (!by_list && curoff < stop) ; pp = ppnext) {
		if (by_list) {
			if (!all &&
			    (pp->offset < start || pp->offset >= stop)) {
				ppnext = TAILQ_NEXT(pp, listq);
				continue;
			}
		} else {
			curoff += PAGE_SIZE;
			if (pp == NULL) {
				if (curoff < stop)
					ppnext = uvm_pagelookup(uobj, curoff);
				continue;
			}
		}

		/*
		 * handle case where we do not need to clean page (either
		 * because we are not clean or because page is not dirty or
		 * is busy):
		 * 
		 * NOTE: we are allowed to deactivate a non-wired active
		 * PG_BUSY page, but once a PG_BUSY page is on the inactive
		 * queue it must stay put until it is !PG_BUSY (so as not to
		 * confuse pagedaemon).
		 */

		if ((flags & PGO_CLEANIT) == 0 || (pp->flags & PG_BUSY) != 0) {
			needs_clean = FALSE;
			if ((flags & (PGO_CLEANIT|PGO_SYNCIO)) ==
			             (PGO_CLEANIT|PGO_SYNCIO))
				need_iosync = TRUE;
		} else {

			/*
			 * freeing: nuke all mappings so we can sync
			 * PG_CLEAN bit with no race
			 */
			if ((pp->flags & PG_CLEAN) != 0 && 
			    (flags & PGO_FREE) != 0 &&
			    (pp->pqflags & PQ_ACTIVE) != 0)
				pmap_page_protect(pp, VM_PROT_NONE);
			if ((pp->flags & PG_CLEAN) != 0 &&
			    pmap_is_modified(pp))
				pp->flags &= ~(PG_CLEAN);
			pp->flags |= PG_CLEANCHK;
			needs_clean = ((pp->flags & PG_CLEAN) == 0);
		}

		/*
		 * if we don't need a clean... load ppnext and dispose of pp
		 */
		if (!needs_clean) {
			if (by_list)
				ppnext = TAILQ_NEXT(pp, listq);
			else {
				if (curoff < stop)
					ppnext = uvm_pagelookup(uobj, curoff);
			}

			if (flags & PGO_DEACTIVATE) {
				if ((pp->pqflags & PQ_INACTIVE) == 0 &&
				    pp->wire_count == 0) {
					pmap_page_protect(pp, VM_PROT_NONE);
					uvm_pagedeactivate(pp);
				}

			} else if (flags & PGO_FREE) {
				if (pp->flags & PG_BUSY) {
					pp->flags |= PG_RELEASED;
				} else {
					pmap_page_protect(pp, VM_PROT_NONE);
					uvm_pagefree(pp);
				}
			}
			/* ppnext is valid so we can continue... */
			continue;
		}

		/*
		 * pp points to a page in the locked object that we are
		 * working on.  if it is !PG_CLEAN,!PG_BUSY and we asked
		 * for cleaning (PGO_CLEANIT).  we clean it now.
		 *
		 * let uvm_pager_put attempted a clustered page out.
		 * note: locked: uobj and page queues.
		 */

		wasclean = FALSE;
		pp->flags |= PG_BUSY;	/* we 'own' page now */
		UVM_PAGE_OWN(pp, "blkio_flush");
		pmap_page_protect(pp, VM_PROT_READ);
		pp_version = pp->version;
ReTry:
		ppsp = pps;
		npages = sizeof(pps) / sizeof(struct vm_page *);

		/* locked: page queues, uobj */
		result = uvm_pager_put(uobj, pp, &ppsp, &npages, 
				       flags | PGO_DOACTCLUST, start, stop);
		/* unlocked: page queues, uobj */

		/*
		 * at this point nothing is locked.   if we did an async I/O
		 * it is remotely possible for the async i/o to complete and 
		 * the page "pp" be freed or what not before we get a chance 
		 * to relock the object.   in order to detect this, we have
		 * saved the version number of the page in "pp_version".
		 */

		/* relock! */
		simple_lock(&uobj->vmobjlock);
		uvm_lock_pageq();

		/*
		 * VM_PAGER_AGAIN: given the structure of this pager, this 
		 * can only happen when  we are doing async I/O and can't
		 * map the pages into kernel memory (pager_map) due to lack
		 * of vm space.   if this happens we drop back to sync I/O.
		 */

		if (result == VM_PAGER_AGAIN) {

			/*
			 * it is unlikely, but page could have been released
			 * while we had the object lock dropped.   we ignore
			 * this now and retry the I/O.  we will detect and
			 * handle the released page after the syncio I/O
			 * completes.
			 */
#ifdef DIAGNOSTIC
			if (flags & PGO_SYNCIO)
	panic("blkio_flush: PGO_SYNCIO return 'try again' error (impossible)");
#endif
			flags |= PGO_SYNCIO;
			goto ReTry;
		}

		/*
		 * the cleaning operation is now done.   finish up.  note that
		 * on error (!OK, !PEND) uvm_pager_put drops the cluster for us.
		 * if success (OK, PEND) then uvm_pager_put returns the cluster
		 * to us in ppsp/npages.
		 */

		/*
		 * for pending async i/o if we are not deactivating/freeing
		 * we can move on to the next page.
		 */

		if (result == VM_PAGER_PEND &&
		    (flags & (PGO_DEACTIVATE|PGO_FREE)) == 0) {

			/*
			 * no per-page ops: refresh ppnext and continue
			 */
			if (by_list) {
				if (pp->version == pp_version)
					ppnext = TAILQ_NEXT(pp, listq);
				else
					ppnext = TAILQ_FIRST(&uobj->memq);
			} else {
				if (curoff < stop)
					ppnext = uvm_pagelookup(uobj, curoff);
			}
			continue;
		}

		/*
		 * need to look at each page of the I/O operation.  we defer 
		 * processing "pp" until the last trip through this "for" loop 
		 * so that we can load "ppnext" for the main loop after we
		 * play with the cluster pages [thus the "npages + 1" in the 
		 * loop below].
		 */

		for (lcv = 0 ; lcv < npages + 1 ; lcv++) {

			/*
			 * handle ppnext for outside loop, and saving pp
			 * until the end.
			 */
			if (lcv < npages) {
				if (ppsp[lcv] == pp)
					continue; /* skip pp until the end */
				ptmp = ppsp[lcv];
			} else {
				ptmp = pp;

				/* set up next page for outer loop */
				if (by_list) {
					if (pp->version == pp_version)
						ppnext = TAILQ_NEXT(pp, listq);
					else
						ppnext = TAILQ_FIRST(
						    &uobj->memq);
				} else {
					if (curoff < stop)
						ppnext = uvm_pagelookup(uobj,
						    curoff);
				}
			}

			/*
			 * verify the page wasn't moved while obj was
			 * unlocked
			 */
			if (result == VM_PAGER_PEND && ptmp->uobject != uobj)
				continue;

			/*
			 * unbusy the page if I/O is done.   note that for
			 * pending I/O it is possible that the I/O op
			 * finished before we relocked the object (in
			 * which case the page is no longer busy).
			 */

			if (result != VM_PAGER_PEND) {
				if (ptmp->flags & PG_WANTED) {
					/* still holding object lock */
					wakeup(ptmp);
				}
				ptmp->flags &= ~(PG_WANTED|PG_BUSY);
				UVM_PAGE_OWN(ptmp, NULL);
				if (ptmp->flags & PG_RELEASED) {
					uvm_unlock_pageq();
					if (!uvn_releasepg(ptmp, NULL)) {
						UVMHIST_LOG(maphist,
							    "released %p",
							    ptmp, 0,0,0);
						return (TRUE);
					}
					uvm_lock_pageq();
					continue;
				} else {
					if ((flags & PGO_WEAK) == 0 &&
					    !(result == VM_PAGER_ERROR &&
					      curproc == uvm.pagedaemon_proc)) {
						ptmp->flags |=
							(PG_CLEAN|PG_CLEANCHK);
						if ((flags & PGO_FREE) == 0) {
							pmap_clear_modify(ptmp);
						}
					}
				}
			}
	  
			/*
			 * dispose of page
			 */

			if (flags & PGO_DEACTIVATE) {
				if ((pp->pqflags & PQ_INACTIVE) == 0 &&
				    pp->wire_count == 0) {
					pmap_page_protect(ptmp, VM_PROT_NONE);
					uvm_pagedeactivate(ptmp);
				}
			} else if (flags & PGO_FREE) {
				if (result == VM_PAGER_PEND) {
					if ((ptmp->flags & PG_BUSY) != 0)
						/* signal for i/o done */
						ptmp->flags |= PG_RELEASED;
				} else {
					if (result != VM_PAGER_OK) {
						printf("blkio_flush: obj=%p, "
						   "offset=0x%llx.  error %d\n",
						    pp->uobject,
						    (long long)pp->offset,
						    result);
						printf("blkio_flush: WARNING: "
						    "changes to page may be "
						    "lost!\n");
						retval = FALSE;
					}
					pmap_page_protect(ptmp, VM_PROT_NONE);
					uvm_pagefree(ptmp);
				}
			}
		}		/* end of "lcv" for loop */
	}		/* end of "pp" for loop */

	uvm_unlock_pageq();
#if 0				/* XXX: until we implement syncer */
	if ((flags & PGO_CLEANIT) && all && wasclean &&
	    LIST_FIRST(&vp->v_dirtyblkhd) == NULL &&
	    (vp->v_flag & VONWORKLST)) {
		vp->v_flag &= ~VONWORKLST;
		LIST_REMOVE(vp, v_synclist);
	}
#endif
#if 0				/* XXX: until we implement synch io */
	if (need_iosync) {
		UVMHIST_LOG(maphist,"  <<DOING IOSYNC>>",0,0,0,0);

		/*
		 * XXX this doesn't use the new two-flag scheme,
		 * but to use that, all i/o initiators will have to change.
		 */

		s = splbio();
		while (vp->v_numoutput != 0) {
			UVMHIST_LOG(ubchist, "waiting for vp %p num %d",
				    vp, vp->v_numoutput,0,0);

			vp->v_flag |= VBWAIT;
			UVM_UNLOCK_AND_WAIT(&vp->v_numoutput,
					    &bobj->u_obj.vmobjlock, 
					    FALSE, "blkio_flush",0);
			simple_lock(&uvn->u_obj.vmobjlock);
		}
		splx(s);
	}
#endif

	/* return, with object locked! */
	UVMHIST_LOG(maphist,"<- done (retval=0x%x)",retval,0,0,0);
	return(retval);
}


/*
 * blkio_put: flush page data to backing store.
 *
 * => object must be locked!   we will _unlock_ it before starting I/O.
 * => flags: PGO_SYNCIO -- use sync. I/O
 * => note: caller must set PG_CLEAN and pmap_clear_modify (if needed)
 */

static int
blkio_put(uobj, pps, npages, flags)
	struct uvm_object *uobj;
	struct vm_page **pps;
	int npages, flags;
{
	struct uvm_blkio_node *bobj = (struct uvm_blkio_node *)uobj;
	int error, error2;
	vaddr_t kva;
	off_t eof, offset, startoffset;
	size_t bytes, iobytes, skipbytes;
	struct vm_page *pg;
	oskit_error_t oskit_err;
	UVMHIST_FUNC("blkio_put"); UVMHIST_CALLED(ubchist);

	simple_unlock(&bobj->u_obj.vmobjlock);

	if (bobj->pagerabsio) {
	    oskit_err = oskit_absio_getsize(bobj->pagerabsio, &eof);
	} else {
	    oskit_err = oskit_blkio_getsize(bobj->pagerblkio, &eof);
	}

	if (oskit_err) {
	    return VM_PAGER_BAD;
	}

	error = error2 = VM_PAGER_OK;

	pg = pps[0];
	startoffset = pg->offset;
	bytes = min(npages << PAGE_SHIFT, eof - startoffset);
	skipbytes = 0;
	KASSERT(bytes != 0);

	kva = uvm_pagermapin(pps, npages, UVMPAGER_MAPIN_WAITOK);

	for (offset = startoffset;
	     bytes > 0;
	     offset += iobytes, bytes -= iobytes) {
#if 0
		lbn = offset >> fs_bshift;
		error = VOP_BMAP(vp, lbn, NULL, &blkno, &run);
		if (error) {
			UVMHIST_LOG(ubchist, "VOP_BMAP() -> %d", error,0,0,0);
			skipbytes += bytes;
			bytes = 0;
			break;
		}
		iobytes = min(((lbn + 1 + run) << fs_bshift) - offset, bytes);
		if (blkno == (daddr_t)-1) {
			skipbytes += iobytes;
			continue;
		}
#endif
		oskit_size_t out_actual;

		iobytes = min(bytes, PAGE_SIZE);
		if (bobj->pagerabsio) {
		    oskit_err = oskit_absio_write(bobj->pagerabsio, (void*)kva, 
						  offset, iobytes, &out_actual);
		} else {
		    oskit_err = oskit_blkio_write(bobj->pagerblkio, (void*)kva,
						  offset, iobytes, &out_actual);
		}

		if (oskit_err) {
		    error = VM_PAGER_ERROR;
		}
#if 0
		/* if it's really one i/o, don't make a second buf */
		if (offset == startoffset && iobytes == bytes) {
			bp = mbp;
		} else {
			s = splbio();
			vp->v_numoutput++;
			bp = pool_get(&bufpool, PR_WAITOK);
			UVMHIST_LOG(ubchist, "vp %p bp %p num now %d",
				    vp, bp, vp->v_numoutput, 0);
			splx(s);
			bp->b_data = (char *)kva +
				(vaddr_t)(offset - pg->offset);
			bp->b_resid = bp->b_bcount = iobytes;
			bp->b_flags = B_BUSY|B_WRITE|B_CALL|B_ASYNC;
			bp->b_iodone = uvm_aio_biodone1;
			bp->b_vp = vp;
			LIST_INIT(&bp->b_dep);
		}
		bp->b_lblkno = 0;
		bp->b_private = mbp;

		/* adjust physical blkno for partial blocks */
		bp->b_blkno = blkno + ((offset - (lbn << fs_bshift)) >>
				       dev_bshift);
		UVMHIST_LOG(ubchist, "vp %p offset 0x%x bcount 0x%x blkno 0x%x",
			    vp, offset, bp->b_bcount, bp->b_blkno);
		VOP_STRATEGY(bp);
#endif
	}
#if 0
	if (skipbytes) {
		UVMHIST_LOG(ubchist, "skipbytes %d", bytes, 0,0,0);
		s = splbio();
		mbp->b_resid -= skipbytes;
		if (mbp->b_resid == 0) {
			biodone(mbp);
		}
		splx(s);
	}
	if (async) {
		UVMHIST_LOG(ubchist, "returning PEND", 0,0,0,0);
		return EINPROGRESS;
	}
	if (bp != NULL) {
		UVMHIST_LOG(ubchist, "waiting for mbp %p", mbp,0,0,0);
		error2 = biowait(mbp);
	}
	if (bioops.io_pageiodone) {
		(*bioops.io_pageiodone)(mbp);
	}
	s = splbio();
	vwakeup(mbp);
	pool_put(&bufpool, mbp);
	splx(s);
#endif
	uvm_pagermapout(kva, npages);
	UVMHIST_LOG(ubchist, "returning, error %d", error,0,0,0);
	return error ? error : error2;
}


/*
 * blkio_get: get pages (synchronously) from backing store
 *
 * => prefer map unlocked (not required)
 * => object must be locked!  we will _unlock_ it before starting any I/O.
 * => flags: PGO_ALLPAGES: get all of the pages
 *           PGO_LOCKED: fault data structures are locked
 * => NOTE: offset is the offset of pps[0], _NOT_ pps[centeridx]
 * => NOTE: caller must check for released pages!!
 */
 
static int
blkio_get(uobj, origoffset, pps, npagesp, centeridx, access_type, advice, flags)
	struct uvm_object *uobj;
	voff_t origoffset;
	struct vm_page **pps;		/* IN/OUT */
	int *npagesp;			/* IN (OUT if PGO_LOCKED) */
	int centeridx;
	vm_prot_t access_type;
	int advice, flags;
{
	struct uvm_blkio_node *bobj = (struct uvm_blkio_node*)uobj;
	oskit_off_t eof, offset, startoffset, endoffset, raoffset;
	int i, error, npages, orignpages, npgs, ridx, pidx;
	vaddr_t kva;
	size_t bytes, iobytes, tailbytes, totalbytes, skipbytes;
	int fs_bsize;
	struct vm_page *pgs[16];			/* XXXUBC 16 */
	boolean_t async = (flags & PGO_SYNCIO) == 0;
	boolean_t write = (access_type & VM_PROT_WRITE) != 0;
	int count = *npagesp;

	UVMHIST_FUNC("blkio_get"); UVMHIST_CALLED(ubchist);
	UVMHIST_LOG(ubchist, "off 0x%x count %d", (int)origoffset, count,0,0);

	/* XXXUBC temp limit */
	if (count > 16) {
		return EINVAL;
	}

	eof = (oskit_off_t)bobj->size;

#ifdef DIAGNOSTIC
	if (centeridx < 0 || centeridx >= count) {
		panic("blkio_get: centeridx %d out of range",
		      centeridx);
	}
	if (origoffset & (PAGE_SIZE - 1) || origoffset < 0) {
		panic("genfs_getpages: offset 0x%x", (int)origoffset);
	}
	if (count < 0) {
		panic("genfs_getpages: count %d < 0", count);
	}
#endif
	
	error = VM_PAGER_OK;

	if (origoffset + (centeridx << PAGE_SHIFT) >= eof &&
	    (flags & PGO_PASTEOF) == 0) {
		if ((flags & PGO_LOCKED) == 0) {
			simple_unlock(&uobj->vmobjlock);
		}
		UVMHIST_LOG(ubchist, "off 0x%x count %d goes past EOF 0x%x",
			    origoffset, count, eof,0);
		return VM_PAGER_BAD;
	}

	/*
	 * For PGO_LOCKED requests, just return whatever's in memory.
	 */

	if (flags & PGO_LOCKED) {
		uvn_findpages(uobj, origoffset, npagesp, pps,
			      UFP_NOWAIT|UFP_NOALLOC|UFP_NORDONLY);
		return pps[centeridx] == NULL ? VM_PAGER_UNLOCK : VM_PAGER_OK;
	}

	/* vnode is VOP_LOCKed, uobj is locked */

#if 0		/* XXXOSKIT: until we implement a syncer process */
	if (write && (vp->v_flag & VONWORKLST) == 0) {
		vn_syncer_add_to_worklist(vp, filedelay);
	}
#endif
					  
	fs_bsize = PAGE_SIZE;
	orignpages = min(count,
			 round_page(eof - origoffset) >> PAGE_SHIFT);
	if (flags & PGO_PASTEOF) {
		orignpages = count;
	}
	npages = orignpages;
	startoffset = origoffset & ~(fs_bsize - 1);
	endoffset = round_page((origoffset + (npages << PAGE_SHIFT)
				+ fs_bsize - 1) & ~(fs_bsize - 1));
	endoffset = min(endoffset, round_page(eof));
	ridx = (origoffset - startoffset) >> PAGE_SHIFT;

	memset(pgs, 0, sizeof(pgs));
	uvn_findpages(uobj, origoffset, &npages, &pgs[ridx], UFP_ALL);

	/*
	 * if PGO_OVERWRITE is set, don't bother reading the pages.
	 * PGO_OVERWRITE also means that the caller guarantees
	 * that the pages already have backing store allocated.
	 */

	if (flags & PGO_OVERWRITE) {
		UVMHIST_LOG(ubchist, "PGO_OVERWRITE",0,0,0,0);

		for (i = 0; i < npages; i++) {
			struct vm_page *pg = pgs[ridx + i];

			if (pg->flags & PG_FAKE) {
				uvm_pagezero(pg);
				pg->flags &= ~(PG_FAKE);
			}
			pg->flags &= ~(PG_RDONLY);
		}
		goto out;
	}

	/*
	 * if the pages are already resident, just return them.
	 */

	for (i = 0; i < npages; i++) {
		struct vm_page *pg = pgs[ridx + i];

		if ((pg->flags & PG_FAKE) ||
		    (write && (pg->flags & PG_RDONLY))) {
			break;
		}
	}
	if (i == npages) {
		UVMHIST_LOG(ubchist, "returning cached pages", 0,0,0,0);
		raoffset = origoffset + (orignpages << PAGE_SHIFT);
		goto raout;
	}

	/*
	 * the page wasn't resident and we're not overwriting,
	 * so we're going to have to do some i/o.
	 * find any additional pages needed to cover the expanded range.
	 */

	if (startoffset != origoffset) {

		/*
		 * XXXUBC we need to avoid deadlocks caused by locking
		 * additional pages at lower offsets than pages we
		 * already have locked.  for now, unlock them all and
		 * start over.
		 */

		for (i = 0; i < npages; i++) {
			struct vm_page *pg = pgs[ridx + i];

			if (pg->flags & PG_FAKE) {
				pg->flags |= PG_RELEASED;
			}
		}
		uvm_page_unbusy(&pgs[ridx], npages);
		memset(pgs, 0, sizeof(pgs));

		UVMHIST_LOG(ubchist, "reset npages start 0x%x end 0x%x",
			    startoffset, endoffset, 0,0);
		npages = (endoffset - startoffset) >> PAGE_SHIFT;
		npgs = npages;
		uvn_findpages(uobj, startoffset, &npgs, pgs, UFP_ALL);
	}
	simple_unlock(&uobj->vmobjlock);

	/*
	 * read the desired page(s).
	 */

	totalbytes = npages << PAGE_SHIFT;
	bytes = min(totalbytes, bobj->size - startoffset);
	tailbytes = totalbytes - bytes;
	skipbytes = 0;

	kva = uvm_pagermapin(pgs, npages, UVMPAGER_MAPIN_WAITOK |
			     UVMPAGER_MAPIN_READ);

	/*
	 * if EOF is in the middle of the last page, zero the part past EOF.
	 */

	if (tailbytes > 0 && (pgs[bytes >> PAGE_SHIFT]->flags & PG_FAKE)) {
		memset((char *)kva + bytes, 0, tailbytes);
	}

	/*
	 * now loop over the pages, reading as needed.
	 */

#if 0
	if (write) {
		lockmgr(&vp->v_glock, LK_EXCLUSIVE, NULL);
	} else {
		lockmgr(&vp->v_glock, LK_SHARED, NULL);
	}
#endif

	iobytes = min(PAGE_SIZE, bytes);
#if 0
	if (offset + iobytes > round_page(offset)) {
		pcount = 1;
		while (pidx + pcount < npages &&
		       pgs[pidx + pcount]->flags & PG_FAKE) {
		    	pcount++;
		}
		iobytes = min(iobytes, (pcount << PAGE_SHIFT) -
			      (offset - trunc_page(offset)));
	}
#endif

	for (offset = startoffset;
	     bytes > 0;
	     offset += iobytes, bytes -= iobytes) {
		oskit_size_t out_actual;
		oskit_error_t oskit_err;

		/*
		 * skip pages which don't need to be read.
		 */

		pidx = (offset - startoffset) >> PAGE_SHIFT;
		while ((pgs[pidx]->flags & PG_FAKE) == 0) {
			size_t b;

#ifdef DEBUG
			if (offset & (PAGE_SIZE - 1)) {
				panic("genfs_getpages: skipping from middle "
				      "of page");
			}
#endif

			b = min(PAGE_SIZE, bytes);
			offset += b;
			bytes -= b;
			skipbytes += b;
			pidx++;
			UVMHIST_LOG(ubchist, "skipping, new offset 0x%x",
				    offset, 0,0,0);
			if (bytes == 0) {
				goto loopdone;
			}
		}

		XPRINTF(OSKIT_DEBUG_PAGER, __FUNCTION__": read offset 0x%lx, "
			"length 0x%lx\n", (long)offset, (long)iobytes);

		UVM_UNLOCK;
		if (bobj->pagerabsio) {
		    oskit_err = oskit_absio_read(bobj->pagerabsio,
					     (char *)kva + offset - startoffset,
					     offset, iobytes, &out_actual);
		} else {
		    oskit_err = oskit_blkio_read(bobj->pagerblkio,
					     (char *)kva + offset - startoffset,
					     offset, iobytes, &out_actual);
		}
		UVM_LOCK;

		if (oskit_err) {
		    	printf(__FUNCTION__": oskit_{absio,blkio}_read "
			       "error %d (offset 0x%lx, length 0x%lx)\n",
			       oskit_err, (long)offset, (long)iobytes);
			error = VM_PAGER_ERROR;
		}
	}

loopdone:
#if 0
	if (skipbytes) {
		s = splbio();
		mbp->b_resid -= skipbytes;
		if (mbp->b_resid == 0) {
			biodone(mbp);
		}
		splx(s);
	}
	if (async) {
		UVMHIST_LOG(ubchist, "returning PEND",0,0,0,0);
		return EINPROGRESS;
	}
	if (bp != NULL) {
		error = biowait(mbp);
	}
#endif
#if 0
	s = splbio();
	pool_put(&bufpool, mbp);
	splx(s);
#endif
	uvm_pagermapout(kva, npages);
	raoffset = offset;

	simple_lock(&uobj->vmobjlock);

	/*
	 * see if we want to start any readahead.
	 * XXXUBC for now, just read the next 128k on 64k boundaries.
	 * this is pretty nonsensical, but it is 50% faster than reading
	 * just the next 64k.
	 */

raout:

#if 0
	/*
	 * XXXOSKIT: Seems this read-ahead code in NetBSD has a problem; 
	 * read pages are not enqueued into the active queue nor inactive 
	 * queue.  Therefore, the page daemon cannot find pages read by
	 * read-ahead.
	 */
	if (!async && !write && ((int)raoffset & 0xffff) == 0 &&
	    PAGE_SHIFT <= 16) {
		int racount;

		racount = 1 << (16 - PAGE_SHIFT);
		(void) blkio_get(uobj, raoffset, NULL, &racount, 0,
				 VM_PROT_READ, 0, 0);
		simple_lock(&uobj->vmobjlock);

		racount = 1 << (16 - PAGE_SHIFT);
		(void) blkio_get(uobj, raoffset + 0x10000, NULL, &racount, 0,
				 VM_PROT_READ, 0, 0);
		simple_lock(&uobj->vmobjlock);
	}
#endif

	/*
	 * we're almost done!  release the pages...
	 * for errors, we free the pages.
	 * otherwise we activate them and mark them as valid and clean.
	 * also, unbusy pages that were not actually requested.
	 */

out:
	if (error) {
	    	uvm_lock_pageq();
		for (i = 0; i < npages; i++) {
			if (pgs[i] == NULL) {
				continue;
			}
			UVMHIST_LOG(ubchist, "examining pg %p flags 0x%x",
				    pgs[i], pgs[i]->flags, 0,0);
			if ((pgs[i]->flags & PG_FAKE) == 0) {
				continue;
			}
			if (pgs[i]->flags & PG_WANTED) {
				wakeup(pgs[i]);
			}
			uvm_pagefree(pgs[i]);
		}
		uvm_unlock_pageq();
		simple_unlock(&uobj->vmobjlock);
		UVMHIST_LOG(ubchist, "returning error %d", error,0,0,0);
		return error;
	}

	UVMHIST_LOG(ubchist, "succeeding, npages %d", npages,0,0,0);
	for (i = 0; i < npages; i++) {
		if (pgs[i] == NULL) {
			continue;
		}
		UVMHIST_LOG(ubchist, "examining pg %p flags 0x%x",
			    pgs[i], pgs[i]->flags, 0,0);
		if (pgs[i]->flags & PG_FAKE) {
			UVMHIST_LOG(ubchist, "unfaking pg %p offset 0x%x",
				    pgs[i], pgs[i]->offset,0,0);
			pgs[i]->flags &= ~(PG_FAKE);
			pmap_clear_modify(pgs[i]);
			pmap_clear_reference(pgs[i]);
		}
		if (write) {
			pgs[i]->flags &= ~(PG_RDONLY);
		}
		if (i < ridx || i >= ridx + orignpages || async) {
			UVMHIST_LOG(ubchist, "unbusy pg %p offset 0x%x",
				    pgs[i], pgs[i]->offset,0,0);
			if (pgs[i]->flags & PG_WANTED) {
				wakeup(pgs[i]);
			}
			if (pgs[i]->wire_count == 0) {
				uvm_pageactivate(pgs[i]);
			}
			pgs[i]->flags &= ~(PG_WANTED|PG_BUSY);
			UVM_PAGE_OWN(pgs[i], NULL);
		}
	}
	simple_unlock(&uobj->vmobjlock);
	if (pps != NULL) {
		memcpy(pps, &pgs[ridx],
		       orignpages * sizeof(struct vm_page *));
	}
	return VM_PAGER_OK;
}
