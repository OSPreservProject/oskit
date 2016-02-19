/*
 * Copyright (c) 1998 The University of Utah and the Flux Group.
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
 * Implements basic HPFQ scheduler stuff for roots and intermediate nodes.
 * Including the SFF and SSF stuff.
 *
 * The user entry-points need to disable interrupts around tree-modifying
 * operations since the transmit-done interrupt, which calls reset-path,
 * can happen anytime.
 */

#include <oskit/hpfq.h>
#include <oskit/io/netio.h>
#include <oskit/io/bufio.h>
#include <oskit/c/assert.h>
#include <oskit/c/alloca.h>
#include <oskit/c/stdlib.h>
#include <oskit/c/string.h>
#include <oskit/dev/dev.h>

#include "hpfq_impl.h"

struct pfq_stats pfq_stats;


/* The pfq_sched_t COM methods. */

static OSKIT_COMDECL
sched_query(pfq_sched_t *_, const oskit_iid_t *iid, void **out_ihandle)
{
	struct pfq_impl *p = (void *)_;

	assert(p && p->p_count);

        if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
            memcmp(iid, &pfq_sched_iid, sizeof(*iid)) == 0) {
                *out_ihandle = &p->p_ioi;
                ++p->p_count;
                return 0;
        }

        *out_ihandle = NULL;
        return OSKIT_E_NOINTERFACE;
}

static OSKIT_COMDECL_U
sched_addref(pfq_sched_t *_)
{
	struct pfq_impl *p = (void *)_;

	assert(p && p->p_count);

        return ++p->p_count;
}

static OSKIT_COMDECL_U
sched_release(pfq_sched_t *_)
{
	struct pfq_impl *p = (void *)_;

	assert(p && p->p_count);

	if (--p->p_count)
		return p->p_count;

	oskit_pqueue_release(p->p_virtq);
	oskit_netio_release(p->p_link);
	free(p);

	return 0;
}

static OSKIT_COMDECL
sched_add_child(pfq_sched_t *_sched, pfq_sched_t *_child, float share)
{
	struct pfq_impl *parent = (void *)_sched;
	struct pfq_impl *child = (void *)_child;

	if (parent == NULL || parent->p_count == 0 ||
	    child == NULL || child->p_count == 0)
		return OSKIT_E_INVALIDARG;

	osenv_intr_disable();

	pfq_sched_addref(_child);
	child->p_byte_charge = (float)PFQ_TOTAL_SHARE / share;
	child->p_parent = parent;

	osenv_intr_enable();
	return 0;
}

static OSKIT_COMDECL
sched_remove_child(pfq_sched_t *_sched, pfq_sched_t *_child)
{
	struct pfq_impl *parent = (void *)_sched;
	struct pfq_impl *child = (void *)_child;

	if (parent == NULL || parent->p_count == 0 ||
	    child == NULL || child->p_count == 0)
		return OSKIT_E_INVALIDARG;

	osenv_intr_disable();

	/* It might or might not be in the virtual queue. */
	oskit_pqueue_remove(parent->p_virtq, (oskit_iunknown_t *)_child);

	child->p_parent = NULL;
	pfq_sched_release(_child);

	osenv_intr_enable();
	return 0;
}

static OSKIT_COMDECL
sched_set_share(pfq_sched_t *_sched, pfq_sched_t *_child, float share)
{
	struct pfq_impl *parent = (void *)_sched;
	struct pfq_impl *child = (void *)_child;

	if (parent == NULL || parent->p_count == 0 ||
	    child == NULL || child->p_count == 0)
		return OSKIT_E_INVALIDARG;

	osenv_intr_disable();

	child->p_byte_charge = (float)PFQ_TOTAL_SHARE / share;

	osenv_intr_enable();
	return 0;
}

static struct pfq_sched_ops sched_ops = {
	sched_query,
	sched_addref,
	sched_release,
	sched_add_child,
	sched_remove_child,
	sched_set_share
};


/*
 * This is called when the time wraps.
 *
 * We need to adjust our times and our peers.
 *
 * The adjustment works by taking the smallest key
 * from our parent's virtq and subtracting that from our's
 * and our peer's times.
 * This is OK since only the relative differences between times matters.
 * It may happen that our parent's virtq is empty (even we are not in there).
 * In this case we just zero our times.
 */
static void
adjust_times(struct pfq_impl *p)
{
	struct x {
		struct pfq_impl	*x_obj;
		pfq_time_t	x_val;
	} *array;
	struct pfq_impl *obj;
	pfq_time_t smallval, val;
	oskit_size_t qsize, i;
	oskit_pqueue_t *virtq;

	pfq_stats.timeadjusts++;

	/* If we are the root, just set our times to zero. */
	if (p->p_parent == NULL) {
		p->p_stime = 0;
		p->p_ftime = 0;
		return;
	}

	/*
	 * Find the smallest val.
	 * If our parent's virtq is emtpy, just zero our time.
	 */
	virtq = p->p_parent->p_virtq;
	obj = (struct pfq_impl *)oskit_pqueue_front(
		virtq,
		&smallval);
	if (obj == NULL) {
		p->p_stime = 0;
		p->p_ftime = 0;
		return;
	}
	pfq_sched_release(&obj->p_ioi);

	/*
	 * Pull the contents of the virtq into an array and munge it.
	 */
	qsize = oskit_pqueue_size(virtq);
	array = alloca(qsize * sizeof(struct x));
	for (i = 0; i < qsize; i++) {
		obj = (struct pfq_impl *)oskit_pqueue_front(virtq, &val);
		obj->p_stime -= smallval;
		obj->p_ftime -= smallval;
		val -= smallval;
		array[i].x_obj = obj;
		array[i].x_val = val;

		oskit_pqueue_remove(virtq, (oskit_iunknown_t*)obj);
	}

	/*
	 * Put them back into the queue.
	 */
	for (i = 0; i < qsize; i++) {
		oskit_pqueue_enqueue(virtq,
                                     (oskit_iunknown_t*)array[i].x_obj,
                                     array[i].x_val);
		pfq_sched_release(&array[i].x_obj->p_ioi);
	}
}

void
_pfq_update_start_finish(struct pfq_impl *p, oskit_bool_t use_finish_time)
{
	oskit_off_t size;
	oskit_error_t err;
	pfq_time_t prev_ftime;

	assert(p);

	p->p_stime = use_finish_time
		     ? p->p_ftime
		     : max(p->p_ftime, p->p_parent ? p->p_parent->p_vtime : 0);

	assert(p->p_logicalq);
	err = oskit_bufio_getsize(p->p_logicalq, &size);
	assert(! err);
	prev_ftime = p->p_ftime;
	p->p_ftime = p->p_stime + (oskit_u32_t)size * p->p_byte_charge;
	if (prev_ftime > p->p_ftime)
		/* Time wrapped. */
		adjust_times(p);

	if (p->p_parent)
		do_update_queue(p->p_parent, p);
}

/*
 * See RESTART-NODE(n) in the paper.
 */
void
_pfq_restart_node(struct pfq_impl *p)
{
	struct pfq_impl *m;

	/* Select next packet for transmission. */
	m = _pfq_select_next(p);

	if (m) {
		assert(p->p_logicalq == NULL);
		p->p_active_child = m;
		p->p_logicalq = m->p_logicalq;
		assert(p->p_logicalq);
		oskit_bufio_addref(p->p_logicalq);

		_pfq_update_start_finish(p, p->p_busy);

		p->p_busy = 1;
		_pfq_update_virtual_time(p);
	}
	else {
		p->p_active_child = NULL;
		p->p_busy = 0;
	}

	/* Restart parent if not root node and not already busy. */
	if (p->p_parent != NULL && p->p_parent->p_logicalq == NULL)
		_pfq_restart_node(p->p_parent);

	/* If hit root node, send the packet. */
	if (p->p_parent == NULL && p->p_logicalq != NULL) {
		oskit_off_t size;
		oskit_error_t err;

		err = oskit_bufio_getsize(p->p_logicalq, &size);
		assert(! err);

		err = oskit_netio_push(p->p_link, p->p_logicalq, (oskit_u32_t)size);
		assert(! err);
		pfq_stats.sent++;
	}
}

/*
 * See RESET-PATH(n) in the paper.
 * This is the implementation for schedulers.
 * There is a different one for leaf nodes.
 */
static void
sched_reset_path(struct pfq_impl *p)
{
	struct pfq_impl *m;

	assert(p->p_logicalq);
	oskit_bufio_release(p->p_logicalq);
	p->p_logicalq = NULL;

	/* Transcend down the hierarchy, resetting the path. */
	m = p->p_active_child;
	p->p_active_child = NULL;
	do_reset_path(m);
	pfq_sched_release(&m->p_ioi);
}

/*
 * This function is called when a node is "restarted".
 * We must select the next child whose queue will be served.
 * We simply pick the one that has the smallest start time AND
 * a packet in its logical queue.
 */
struct pfq_impl *
_pfq_select_next(struct pfq_impl *p)
{
	struct pfq_impl *q = NULL;

	/* Pick child with smallest start time (PFQ). */
	while (oskit_pqueue_size(p->p_virtq) != 0) {
		q = (struct pfq_impl *)oskit_pqueue_front(p->p_virtq, NULL);
		if (q->p_logicalq)
			break;
		else {
			pfq_sched_release(&q->p_ioi);
			oskit_pqueue_remove(p->p_virtq, (oskit_iunknown_t *)q);
			q = NULL;
		}
	}

	return q;
}

void
_pfq_update_virtual_time(struct pfq_impl *p)
{
	pfq_time_t val;
	pfq_sched_t *obj;

	obj = (pfq_sched_t *)oskit_pqueue_front(p->p_virtq, &val);
	pfq_sched_release(obj);
	p->p_vtime = val;
}

/*
 * Update the virtual queue for the Smallest Finish time First algorithm.
 */
static void
sff_update_queue(struct pfq_impl *parent, struct pfq_impl *child)
{
	oskit_pqueue_remove(parent->p_virtq, (oskit_iunknown_t *)child);
	oskit_pqueue_enqueue(parent->p_virtq, (oskit_iunknown_t *)child,
			     child->p_ftime);
}

/*
 * Update the virtual queue for the Smallest Start time First algorithm.
 */
static void
ssf_update_queue(struct pfq_impl *parent, struct pfq_impl *child)
{
	oskit_pqueue_remove(parent->p_virtq, (oskit_iunknown_t *)child);
	oskit_pqueue_enqueue(parent->p_virtq, (oskit_iunknown_t *)child,
			     child->p_stime);
}


static oskit_error_t
create_default(struct pfq_impl **out_p)
{
	struct pfq_impl *p;
	oskit_error_t err;

	/*
	 * This node isn't part of the tree yet -- so we don't need to
	 * disable interrupts.
	 */

	p = malloc(sizeof *p);
	if (p == NULL)
		return OSKIT_E_OUTOFMEMORY;

	/*
	 * Set up some reasonable defaults.
	 */
	p->p_ioi.ops = &sched_ops;
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
	p->p_reset_path = sched_reset_path;
	p->p_update_queue = NULL;

	err = oskit_pqueue_create(&p->p_virtq);
	if (err) {
		free(p);
		return err;
	}

	*out_p = p;
	return 0;
}

/*
 * Create an SFF intermediate node.
 */
oskit_error_t
pfq_sff_create(pfq_sched_t **out_sched)
{
	struct pfq_impl *p;
	oskit_error_t err;

	err = create_default(&p);
	if (err)
		return err;

	p->p_update_queue = sff_update_queue;

	*out_sched = &p->p_ioi;
	return 0;
}

/*
 * Create an SSF intermediate node.
 */
oskit_error_t
pfq_ssf_create(pfq_sched_t **out_sched)
{
	struct pfq_impl *p;
	oskit_error_t err;

	err = create_default(&p);
	if (err)
		return err;

	p->p_update_queue = ssf_update_queue;

	*out_sched = &p->p_ioi;
	return 0;
}

/*
 * Create an SFF root node.
 * Just like an intermediate node but has a p_link.
 */
oskit_error_t
pfq_sff_create_root(oskit_netio_t *link, pfq_sched_t **out_sched)
{
	struct pfq_impl *p;
	oskit_error_t err;

	err = pfq_sff_create((pfq_sched_t **)&p);
	if (err)
		return err;

	p->p_link = link;
	oskit_netio_addref(link);

	*out_sched = (pfq_sched_t *)p;
	return 0;
}

/*
 * Create an SSF root node.
 * Just like an intermediate node but has a p_link.
 */
oskit_error_t
pfq_ssf_create_root(oskit_netio_t *link, pfq_sched_t **out_sched)
{
	struct pfq_impl *p;
	oskit_error_t err;

	err = pfq_ssf_create((pfq_sched_t **)&p);
	if (err)
		return err;

	p->p_link = link;
	oskit_netio_addref(link);

	*out_sched = (pfq_sched_t *)p;
	return 0;
}

void
pfq_reset_path(pfq_sched_t *root)
{
	struct pfq_impl *p = (void *)root;

	assert(p && p->p_count);

	/*
	 * This is called from an interrupt handler so we don't need
	 * to turn them off -- this routine is exactly what we are
	 * guarding against.
	 */

	/*
	 * XXX We are supposed to be called at transmit-done time (when
	 * p_logicalq is guaranteed to be non-NULL) but may be called
	 * at other times.
	 * In Linux, the net_bh routine (which calls my_dev_transmit
	 * which calls this) is called when packets are received and
	 * at transmit-done time.
	 */
	if (p->p_logicalq == NULL)
		return;

	do_reset_path(p);
}
