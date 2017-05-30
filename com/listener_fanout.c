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
 * implement a list of listeners and the code to notify them - this can be
 * used by any COM interface implementation maintaining listeners
 */
#include <oskit/com/listener.h>
#include <oskit/com/listener_fanout.h>
#include <oskit/c/string.h>
#include <oskit/c/stdlib.h>
#include <oskit/c/malloc.h>
#include <oskit/c/assert.h>

/*
 * We're using a simple linked list.
 */
struct listener_instance {
        struct listener_instance *next;
        struct oskit_listener     *listener;
};

/*
 * the object, and the list of listeners
 */
typedef struct listener_mgr {
	oskit_listener_fanout_t	io;	/* COM interface */
	unsigned count;			/* count */

        struct listener_instance *head;
} listener_mgr_t;

static OSKIT_COMDECL
listener_query(oskit_listener_fanout_t *io, const oskit_iid_t *iid, 
	void **out_ihandle)
{
        listener_mgr_t *li = (listener_mgr_t *)io;

        if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
            memcmp(iid, &oskit_listener_iid, sizeof(*iid)) == 0 ||
            memcmp(iid, &oskit_listener_fanout_iid, sizeof(*iid)) == 0) {
                *out_ihandle = &li->io;
                ++li->count;
                return 0;
        }

        *out_ihandle = NULL;
        return OSKIT_E_NOINTERFACE;
}

static OSKIT_COMDECL_U
listener_addref(oskit_listener_fanout_t *io)
{
        listener_mgr_t *li = (listener_mgr_t *)io;

        if (li->count == 0)
                return OSKIT_E_INVALIDARG;

        return ++li->count;
}

/*
 * destroy an listener_mgr
 */
static OSKIT_COMDECL_U
listener_release(oskit_listener_fanout_t *io)
{
        listener_mgr_t *li = (listener_mgr_t *)io;
        unsigned newcount;

        if (li == NULL || li->count == 0)
                return OSKIT_E_INVALIDARG;

        newcount = li->count - 1;
        if (newcount == 0) {
		while (li->head)
			oskit_listener_fanout_remove(io, li->head->listener);
                free(li);
		return 0;
        }

        return li->count = newcount;
}

/*
 * add a listener to list
 */
static OSKIT_COMDECL
listener_add(oskit_listener_fanout_t *io, struct oskit_listener *l)
{
        struct listener_mgr *mgr = (listener_mgr_t *)io;
	struct listener_instance *li;
	li = malloc(sizeof(*li));
	if (!li)
		return OSKIT_E_OUTOFMEMORY;
	memset(li, 0, sizeof *li);
	/* set listener and insert in front of list */
	oskit_listener_addref(l);
	li->listener = l;
	li->next = mgr->head;
	mgr->head = li;
	return 0;
}

/*
 * remove a listener from list
 * return 0 on success, an error code if object wasn't on list
 */
static OSKIT_COMDECL
listener_remove(oskit_listener_fanout_t *io, struct oskit_listener *l0)
{
        struct listener_mgr *mgr = (listener_mgr_t *)io;
	struct listener_instance **l, *ltemp;
	
        for (l = &mgr->head; *l; l = &(*l)->next)
                if ((*l)->listener == l0) {
                        oskit_listener_release(l0);
			ltemp = (*l)->next;
			free(*l);
                        *l = ltemp; 
                        return 0;
                }
	return OSKIT_E_INVALIDARG;	/* is that the right return code? */
}

/*
 * Notify all listeners
 */
static OSKIT_COMDECL
listener_notify(oskit_listener_fanout_t *io, oskit_iunknown_t *obj)
{
	struct listener_mgr *mgr = (listener_mgr_t *)io;
	struct listener_instance *l;
	oskit_error_t	rc, rc1 = 0;

	for (l = mgr->head; l; l = l->next) {
		rc = oskit_listener_notify(l->listener, obj);
		if (rc)
			rc1 = rc;
	}
	return rc1;
}

/*
 * Count number of listeners
 */
static OSKIT_COMDECL_U
listener_count(oskit_listener_fanout_t *io)
{
	struct listener_mgr *mgr = (listener_mgr_t *)io;
	struct listener_instance *l;
	int count = 0;

	for (l = mgr->head; l; l = l->next)
		count++;

	return count;
}

struct oskit_listener_fanout_ops oskit_listener_fanout_ops = {
        listener_query,
        listener_addref,
        listener_release,
        listener_notify,
        listener_add,
        listener_remove,
        listener_count
};


/*
 * create a new listener_mgr
 */
oskit_listener_fanout_t *
oskit_create_listener_fanout()
{
	listener_mgr_t	*newmgr;

	newmgr = malloc(sizeof(*newmgr));
	if (!newmgr)
		return 0;
	memset(newmgr, 0, sizeof *newmgr);
	newmgr->head = 0;
	newmgr->io.ops = &oskit_listener_fanout_ops;
	newmgr->count = 1;

	return &newmgr->io;
}

