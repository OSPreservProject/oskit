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

#ifndef _OSKIT_HPFQ_H_
#define _OSKIT_HPFQ_H_

#include <oskit/io/netio.h>


/*
 * PFQ Scheduler interface,
 * IID 4aa7dfb9-7c74-11cf-b500-08000953adc2
 */
struct pfq_sched {
	struct pfq_sched_ops *ops;
};
typedef struct pfq_sched pfq_sched_t;

struct pfq_sched_ops {
	OSKIT_COMDECL_IUNKNOWN(pfq_sched_t)

	OSKIT_COMDECL (*add_child)(pfq_sched_t *sched, pfq_sched_t *child,
				   float share);
	OSKIT_COMDECL (*remove_child)(pfq_sched_t *sched, pfq_sched_t *child);
	OSKIT_COMDECL (*set_share)(pfq_sched_t *sched, pfq_sched_t *child,
				   float share);
};

extern const struct oskit_guid pfq_sched_iid;
#define PFQ_SCHED_IID OSKIT_GUID(0x4aa7dfb9, 0x7c74, 0x11cf, \
		      0xb5, 0x00, 0x08, 0x00, 0x09, 0x53, 0xad, 0xc2)

#define pfq_sched_query(io, iid, out_ihandle) \
	((io)->ops->query((pfq_sched_t *)(io), (iid), (out_ihandle)))
#define pfq_sched_addref(io) \
	((io)->ops->addref((pfq_sched_t *)(io)))
#define pfq_sched_release(io) \
	((io)->ops->release((pfq_sched_t *)(io)))
#define pfq_sched_add_child(sched, child, share) \
	((sched)->ops->add_child((pfq_sched_t *)(sched), (child), (share)))
#define pfq_sched_remove_child(sched, child) \
	((sched)->ops->remove_child((pfq_sched_t *)(sched), (child)))
#define pfq_sched_set_share(sched, child, share) \
	((sched)->ops->set_share((pfq_sched_t *)(sched), (child), (share)))


/*
 * PFQ Leaf Node interface,
 * IID 4aa7dfba-7c74-11cf-b500-08000953adc2
 */
struct pfq_leaf {
	struct pfq_leaf_ops *ops;
};
typedef struct pfq_leaf pfq_leaf_t;

struct pfq_leaf_ops {
	OSKIT_COMDECL_IUNKNOWN(pfq_leaf_t)

	/*** Operations inherited from pfq_sched. ***/
	OSKIT_COMDECL (*add_child)(pfq_leaf_t *leaf, pfq_sched_t *child,
				   float share);
	OSKIT_COMDECL (*remove_child)(pfq_leaf_t *leaf, pfq_sched_t *child);
	OSKIT_COMDECL (*set_share)(pfq_leaf_t *leaf, pfq_sched_t *child,
				   float share);

	/*
	 * Gets an oskit_netio_t from a pfq_leaf.
	 * The oskit_netio_t has a push method that does the "arrive" algorithm.
	 * The pfq_leaf must have been parented via pfq_add_child or this
	 * call will fail with OSKIT_E_INVALIDARG.
	 * The push/arrive operates on a bounded queue and for now
	 * will be nonblocking and just drop everything sent when the
	 * queue is full.
	 */
	OSKIT_COMDECL (*get_netio)(pfq_leaf_t *leaf, oskit_netio_t **out_netio);
};

extern const struct oskit_guid pfq_leaf_iid;
#define PFQ_LEAF_IID OSKIT_GUID(0x4aa7dfba, 0x7c74, 0x11cf, \
		     0xb5, 0x00, 0x08, 0x00, 0x09, 0x53, 0xad, 0xc2)

#define pfq_leaf_query(io, iid, out_ihandle) \
	((io)->ops->query((pfq_leaf_t *)(io), (iid), (out_ihandle)))
#define pfq_leaf_addref(io) \
	((io)->ops->addref((pfq_leaf_t *)(io)))
#define pfq_leaf_release(io) \
	((io)->ops->release((pfq_leaf_t *)(io)))
#define pfq_leaf_add_child(leaf, child, share) \
	((leaf)->ops->add_child((pfq_leaf_t *)(leaf), (child), (share)))
#define pfq_leaf_remove_child(leaf, child) \
	((leaf)->ops->remove_child((pfq_leaf_t *)(leaf), (child)))
#define pfq_leaf_set_share(leaf, child, share) \
	((leaf)->ops->set_share((pfq_leaf_t *)(leaf), (child), (share)))
#define pfq_leaf_get_netio(leaf, out_netio) \
	((leaf)->ops->get_netio((pfq_leaf_t *)(leaf), (out_netio)))


/* Constructors */

/*
 * Create a pfq_leaf that contains an oskit_netio_t with a push method that
 * does the "arrive" algorithm.
 * This netio can be accessed by doing pfq_leaf_get_netio and then
 * used like a normal netio.
 */
oskit_error_t pfq_leaf_create(pfq_leaf_t **out_leaf);

/*
 * Create a PFQ scheduler implementing Smallest Finish time First.
 */
oskit_error_t pfq_sff_create_root(oskit_netio_t *link, pfq_sched_t **out_sched);
oskit_error_t pfq_sff_create(pfq_sched_t **out_sched);

/*
 * Create a PFQ scheduler implementing Smallest Start time First.
 */
oskit_error_t pfq_ssf_create_root(oskit_netio_t *link, pfq_sched_t **out_sched);
oskit_error_t pfq_ssf_create(pfq_sched_t **out_sched);


/* Other entry points. */

/*
 * Resets the path from the root to the sent packet.
 * This is meant to be called from the transfer-done interrupt handler
 * (or equivalent).
 */
void pfq_reset_path(pfq_sched_t *root);

/*
 * Print out some stats.
 */
void pfq_dump_stats(void);

/*
 * The root of the PFQ tree.
 * (XXX this would be better tied to the root's netio.)
 */
extern pfq_sched_t *oskit_pfq_root;

/*
 * The reset_path function.
 * This should be set to `pfq_reset_path' by the client OS.
 */
extern void (*oskit_pfq_reset_path)(pfq_sched_t *);

#endif /* _OSKIT_HPFQ_H_ */
