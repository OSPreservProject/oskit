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

#include "sproc_internal.h"

/*
 *  Handler for UVM traps.  Handle page fault errors.
 */
extern void
oskit_sproc_uvm_handler(oskit_vmspace_t vm, int signo, struct trap_state *ts)
{
    struct oskit_sproc *sproc;
    struct oskit_sproc_thread *sthread;
    int rc;

    sthread =
	(struct oskit_sproc_thread*)pthread_getspecific(oskit_sproc_thread_key);
    assert(sthread);
    sproc = sthread->st_process;
    if (sproc->sp_desc->sd_handler) {
	rc = (*sproc->sp_desc->sd_handler)(sthread, signo, ts->trapno, ts);
	if (rc) {
	    OSKIT_SPROC_RETURN(sthread, -1);
	}
	return;
    }

    oskit_sendsig(signo, ts);
}
