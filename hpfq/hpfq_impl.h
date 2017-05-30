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
#ifndef _HPFQ_HPFQ_IMPL_H_
#define _HPFQ_HPFQ_IMPL_H_

#include <oskit/hpfq.h>
#include <oskit/io/netio.h>
#include <oskit/com/pqueue.h>
#include <oskit/com/queue.h>

/*
 * The amount charged per packet is computed as length * p_byte_charge,
 * and the p_byte_charge is computed as PFQ_TOTAL_SHARE/share
 * where share represents a percentage (e.g., 0.5 for 50%).
 * Thus PFQ_TOTAL_SHARE limits the precision of sharing.
 */
#define PFQ_TOTAL_SHARE 1000

/*
 * Size of the physical queues used by the leaf nodes.
 */
#define PFQ_LEAF_QUEUE_LENGTH 100

/*
 * The type used for times.
 * MUST AGREE WITH THE IMPLEMENTATION OF OSKIT_PQUEUE_T USED.
 */
typedef oskit_pqueue_key_t pfq_time_t;

/*
 * This is the data structure used to implement PFQ Schedulers as well
 * as PFQ leaf-nodes, hence the union.
 * Object-oriented C at its best...
 */
struct pfq_impl {
	pfq_sched_t	p_ioi;		/* may be a pfq_leaf_t, same size... */
	oskit_u32_t	p_count;

	oskit_netio_t	*p_link;	/* the physical link */
	struct pfq_impl	*p_active_child;/* pointer to active child */
	oskit_u32_t	p_byte_charge;	/* per-byte charge */
	oskit_bool_t	p_busy;		/* are we busy? */
	struct pfq_impl	*p_parent;	/* pointer to parent */
	oskit_bufio_t	*p_logicalq;	/* Logical "queue" (just first elem) */
	pfq_time_t	p_vtime;	/* virtual time */
	pfq_time_t	p_stime;	/* start time for p_lqueue */
	pfq_time_t	p_ftime;	/* finish time for p_lqueue */

	/* Here are our virtual methods. */
	void		(*p_reset_path)(struct pfq_impl *p);
	void		(*p_update_queue)(struct pfq_impl *p,
					  struct pfq_impl *child);

	union {
		/* PFQ schedulers keep their children in a priority queue. */
		oskit_pqueue_t	*u_virtq;

		/* Leaf nodes have real packet queues. */
		oskit_queue_t	*u_packq;
	} u;
#define p_virtq u.u_virtq
#define p_packq u.u_packq
};

/*
 * Contains various stats.
 */
struct pfq_stats {
	oskit_u32_t	arrived;	/* packets that were
					   attempted to be sent */
	oskit_u32_t	qfull;		/* packets dropped
					   because the interface queue
					   was full */
	oskit_u32_t	sent;		/* packets put on the wire */
	oskit_u32_t	timeadjusts;	/* times the timers overflowed
					   and had to be adjusted */
};

extern struct pfq_stats pfq_stats;

#define do_reset_path(p)	((p)->p_reset_path(p))
#define do_update_queue(p,c)	((p)->p_update_queue(p,c))

#undef max
#define max(a,b)	((a)<(b)?(b):(a))

void _pfq_update_start_finish(struct pfq_impl *p, oskit_bool_t use_finish_time);
void _pfq_restart_node(struct pfq_impl *p);
struct pfq_impl * _pfq_select_next(struct pfq_impl *p);
void _pfq_update_virtual_time(struct pfq_impl *p);

#endif /* _HPFQ_HPFQ_IMPL_H__ */
