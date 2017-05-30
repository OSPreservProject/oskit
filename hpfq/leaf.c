/*
 * Copyright (c) 1998, 2000 The University of Utah and the Flux Group.
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
 * Implements pfq_leaf operations.
 *
 * The user entry-points need to disable interrupts around tree-modifying
 * operations since the transmit-done interrupt, which calls reset-path,
 * can happen anytime.
 */

#include <oskit/hpfq.h>
#include <oskit/io/netio.h>
#include <oskit/c/assert.h>
#include <oskit/c/stdlib.h>
#include <oskit/c/string.h>
#include <oskit/com/queue.h>
#include <oskit/dev/dev.h>

#include "hpfq_impl.h"


/* The special implementation of oskit_netio_t for PFQ leaf nodes. */

struct netio_impl {
	oskit_netio_t	n_ioi;
	oskit_u32_t	n_count;

	struct pfq_impl	*n_leaf;
};

static OSKIT_COMDECL
netio_query(oskit_netio_t *_, const oskit_iid_t *iid, void **out_ihandle)
{
	struct netio_impl *n = (void *)_;

	assert(n && n->n_count);

        if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
            memcmp(iid, &oskit_netio_iid, sizeof(*iid)) == 0) {
                *out_ihandle = &n->n_ioi;
                ++n->n_count;
                return 0;
        }

        *out_ihandle = NULL;
        return OSKIT_E_NOINTERFACE;
}

static OSKIT_COMDECL_U
netio_addref(oskit_netio_t *_)
{
	struct netio_impl *n = (void *)_;

	assert(n && n->n_count);

        return ++n->n_count;
}

static OSKIT_COMDECL_U
netio_release(oskit_netio_t *_)
{
	struct netio_impl *n = (void *)_;

	assert(n && n->n_count);

	if (--n->n_count)
		return n->n_count;

	free(n);

	return 0;
}

/*
 * This is what you're looking for.
 * It does the ARRIVE(i, Packet) algorithm from the paper.
 */
static OSKIT_COMDECL
netio_push(oskit_netio_t *_, oskit_bufio_t *b, oskit_size_t size)
{
	struct netio_impl *n = (void *)_;
	struct pfq_impl *p = n->n_leaf;
	oskit_error_t err;

	assert(n && n->n_count);

	osenv_intr_disable();

	pfq_stats.arrived++;

	/* Append to end of physical queue. */
	err = oskit_queue_enqueue(p->p_packq, b, 0);
	if (err == OSKIT_E_FAIL)
		pfq_stats.qfull++;

	/* Done if p_logicalq is non-empty. */
	if (p->p_logicalq)
		goto done;

	/* Otherwise, this packet becomes the head of the p_logicalq queue. */
	p->p_logicalq = b;
	oskit_bufio_addref(b);

	/* Update start and finish times. */
	_pfq_update_start_finish(p, 0);

	/* Restart our parent if they're not watching T.V. */
	if (! p->p_parent->p_busy)
		_pfq_restart_node(p->p_parent);

 done:
	osenv_intr_enable();
	return 0;
}

/*
 * Unimplemented bufio allocator
 */
static OSKIT_COMDECL
netio_alloc_bufio(oskit_netio_t *io, oskit_size_t size,
		oskit_bufio_t **out_bufio)
{
	return OSKIT_E_NOTIMPL;
}

static struct oskit_netio_ops leaf_netio = {
	netio_query,
	netio_addref,
	netio_release,
	netio_push,
	netio_alloc_bufio
};


/* The pfq_leaf_t COM methods. */

static OSKIT_COMDECL
leaf_query(pfq_leaf_t *_, const oskit_iid_t *iid, void **out_ihandle)
{
	struct pfq_impl *p = (void *)_;

	assert(p && p->p_count);

        if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
            memcmp(iid, &pfq_leaf_iid, sizeof(*iid)) == 0) {
                *out_ihandle = &p->p_ioi;
                ++p->p_count;
                return 0;
        }

        *out_ihandle = NULL;
        return OSKIT_E_NOINTERFACE;
}

static OSKIT_COMDECL_U
leaf_addref(pfq_leaf_t *_)
{
	struct pfq_impl *p = (void *)_;

	assert(p && p->p_count);

        return ++p->p_count;
}

static OSKIT_COMDECL_U
leaf_release(pfq_leaf_t *_)
{
	struct pfq_impl *p = (void *)_;

	assert(p && p->p_count);

	if (--p->p_count)
		return p->p_count;

	oskit_queue_release(p->p_packq);
	free(p);

	return 0;
}

static OSKIT_COMDECL
leaf_get_netio(pfq_leaf_t *_, oskit_netio_t **out_netio)
{
	struct pfq_impl *p = (void *)_;
	struct netio_impl *n;

	assert(p && p->p_count);

	/* Must have been parented. */
	if (p->p_parent == NULL)
		return OSKIT_E_INVALIDARG;

	n = malloc(sizeof *n);
	if (n == NULL)
		return OSKIT_E_OUTOFMEMORY;

	n->n_ioi.ops = &leaf_netio;
	n->n_count = 1;
	n->n_leaf = p;

	*out_netio = &n->n_ioi;
	return 0;
}

static OSKIT_COMDECL
notimpl(void)
{
	return OSKIT_E_NOTIMPL;
}

static struct pfq_leaf_ops leaf_ops = {
	leaf_query,
	leaf_addref,
	leaf_release,
	(void *)notimpl,		/* add_child */
	(void *)notimpl,		/* remove_child */
	(void *)notimpl,		/* set_share */
	leaf_get_netio
};

/*
 * See RESET-PATH(n) in the paper.
 * This is the implementation for leaf nodes.
 * There is a different one for schedulers.
 */
static void
leaf_reset_path(struct pfq_impl *p)
{
	oskit_bufio_t *b;

	assert(p->p_logicalq);
	oskit_bufio_release(p->p_logicalq);
	p->p_logicalq = NULL;

	/* Dequeue and restart. */
	oskit_queue_dequeue(p->p_packq, &b);
	oskit_bufio_release(b);
	if (oskit_queue_size(p->p_packq) != 0) {
		oskit_queue_front(p->p_packq, &p->p_logicalq);
		_pfq_update_start_finish(p, 1);
	}
	_pfq_restart_node(p->p_parent);
}


oskit_error_t
pfq_leaf_create(pfq_leaf_t **out_leaf)
{
	struct pfq_impl *p;
	oskit_error_t err;

	/*
	 * This leaf isn't part of the tree yet -- so we don't need to
	 * disable interrupts.
	 */

	p = malloc(sizeof *p);
	if (p == NULL)
		return OSKIT_E_OUTOFMEMORY;

	/*
	 * Set up some reasonable defaults.
	 */
	p->p_ioi.ops = (struct pfq_sched_ops *)&leaf_ops;
	p->p_count = 1;
	p->p_link = NULL;
	p->p_active_child = NULL;
	p->p_byte_charge = PFQ_TOTAL_SHARE/1;		/* default to 100% */
	p->p_busy = 0;
	p->p_parent = NULL;
	p->p_logicalq = NULL;
	p->p_vtime = 0;
	p->p_stime = 0;
	p->p_ftime = 0;
	p->p_reset_path = leaf_reset_path;
	p->p_update_queue = NULL;

	err = oskit_bounded_com_queue_create(PFQ_LEAF_QUEUE_LENGTH,
					     &p->p_packq);
	if (err) {
		free(p);
		return err;
	}

	*out_leaf = (pfq_leaf_t *)&p->p_ioi;
	return 0;
}
