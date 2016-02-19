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

#include "oskit_uvm_internal.h"

static void
pagedaemon_thread(void *arg)
{
    struct oskit_vmspace *vm = (struct oskit_vmspace*)arg;
    UVM_LOCK;
    oskit_uvm_vmspace_set(vm);
    uvm_pageout(0);
    /* Never returns */
}

extern void
oskit_uvm_start_pagedaemon()
{
    pthread_t thread;
    struct oskit_vmspace *vm;

    /*
     * NetBSD's UVM code assumes the page daemon has own proc structure.
     * You can see several statements like if (curproc == uvm.pagedaemon_proc)
     * in the UVM code.
     */
    oskit_uvm_vmspace_alloc(&vm);
    vm->vm_proc.p_vmspace = &vmspace0;
    pthread_create(&thread, NULL, (void *(*)__P((void*)))pagedaemon_thread,
		   (void*)vm);

#if 1
    {
	struct oskit_uvm_per_thread *pt;
	struct lwp *lwp;
	while (1) {
	    pt = oskit_pthread_getspecific(thread, oskit_uvm_per_thread_key);
	    if (pt && (lwp = &pt->pt_lwp) && lwp->p_stat == SSLEEP) {
		break;
	    }
	    sched_yield();
	}
    }
#else
    /* Wait until the page daemon starts */
    while (vm->vm_proc.p_stat != SSLEEP) {
        sched_yield();
    }
#endif
}


