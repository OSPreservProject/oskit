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
#include <oskit/com/listener_mgr.h>
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
        struct oskit_iunknown            *obj;
        struct listener_instance        *head;
} listener_mgr_t;

/*
 * create a new listener_mgr
 */
listener_mgr_t	*
oskit_create_listener_mgr(struct oskit_iunknown *obj)
{
	listener_mgr_t	*newmgr;

	newmgr = malloc(sizeof(*newmgr));
	if (!newmgr)
		return 0;
	memset(newmgr, 0, sizeof *newmgr);
	newmgr->obj = obj;
	newmgr->head = 0;
	return newmgr;
}

/*
 * destroy an listener_mgr
 */
void
oskit_destroy_listener_mgr(listener_mgr_t *mgr)
{
	/*
	 * remove listener if there are any left
	 */
	while (mgr->head)
		oskit_listener_mgr_remove(mgr, mgr->head->listener);
	free(mgr);
}

/*
 * add a listener to list
 */
oskit_error_t 
oskit_listener_mgr_add(listener_mgr_t  *mgr, struct oskit_listener *l)
{
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
oskit_error_t
oskit_listener_mgr_remove(listener_mgr_t  *mgr, struct oskit_listener *l0)
{
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
void 
oskit_listener_mgr_notify(listener_mgr_t  *mgr)
{
	struct listener_instance *l;
	for (l = mgr->head; l; l = l->next)
		oskit_listener_notify(l->listener, mgr->obj);
}

/*
 * Count number of listeners
 */
int 
oskit_listener_mgr_count(struct listener_mgr  *mgr)
{
	int count = 0;
	struct listener_instance *l;
	for (l = mgr->head; l; l = l->next)
		count++;

	return count;
}

