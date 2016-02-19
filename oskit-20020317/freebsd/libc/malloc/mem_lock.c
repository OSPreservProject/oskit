/*
 * Copyright (c) 1996, 1998, 1999 University of Utah and the Flux Group.
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
 * This defines the oskit's default memory locking for multi-threaded
 * malloc. 
 */

#include "malloc.h"
#include "stdlib.h"
#include <oskit/c/environment.h>

#ifndef THREAD_SAFE
/*
 * Single threaded does nothing by default, depending on linking in
 * the kernel version if memory allocation needs to be protected
 * against interrupt handlers that allocate memory. This seems wrong.
 * Whats the right thing to do here?
 */
void mem_unlock(void)
{
}

void mem_lock(void)
{
}

void mem_lock_init(void)
{
}

#else	/* THREAD_SAFE */

/*
 * Thread safe version.
 */
#include <oskit/com/services.h>
#include <oskit/com/lock_mgr.h>
#include <oskit/com/lock.h>

static oskit_lock_t	*memlock;

void mem_unlock(void)
{
	if (!memlock)
		return;

	oskit_lock_unlock(memlock);
}

void mem_lock(void)
{
	if (!memlock)
		return;

	oskit_lock_lock(memlock);
}

/*
 * Query the registry for the lock_mgr. If its null, then no memory locking
 * is needed. Otherwise create a lock.
 */
#ifdef KNIT
extern oskit_lock_mgr_t	*oskit_lock_mgr;

oskit_error_t
init(void)
{
	return oskit_lock_mgr_allocate_critical_lock(oskit_lock_mgr, &memlock);
}

oskit_error_t
fini(void)
{
        if (memlock) {
                return oskit_lock_release(memlock);
                memlock = 0;
        }
}
#else /* !KNIT */
void mem_lock_init(void)
{
	oskit_lock_mgr_t	*lock_mgr;

	/*
	 * Allow only once!
	 */
	if (memlock)
		return;

	if (oskit_library_services_lookup(&oskit_lock_mgr_iid,
					  (void *) &lock_mgr))
		panic("mem_lock_init: oskit_library_services_lookup");

	if (!lock_mgr)
		return;

	if (oskit_lock_mgr_allocate_critical_lock(lock_mgr, &memlock))
		panic("mem_lock_init: oskit_lock_mgr_allocate_lock");
}
#endif /* !KNIT */
#endif /* THREAD_SAFE */
