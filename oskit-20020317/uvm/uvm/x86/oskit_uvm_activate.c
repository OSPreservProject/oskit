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

struct user			oskit_uvm_user;

/*
 * Switch the vmspace and change related variables.
 *
 * To understand the `right' way to update NetBSD's curpcb variable
 * is a bit complicated so I note here.
 * 
 * curpcb is x86 port specific variable and used in pmap.c, locore.s 
 * and pfault.c.  Our strategy is,
 * when a thread calls oskit_uvm_vmspace_set(), dynamically assign pcb
 * structure (that is in struct user) and remember it using thread
 * specific data.  If a thread does not call oskit_uvm_vmspace_set(),
 * shared DEFAULT_PCB is assigned for the thread.  I believe this
 * default pcb may be used only in pmap_{un,}map_ptes for locking purpose.
 * pcb is also accessed from our page fault handler.  The
 * handler distinguishes whether curpcb is default one or not.  
 */ 
extern void
oskit_uvm_activate_locked(oskit_vmspace_t vm)
{
    struct oskit_uvm_per_thread *pt;

    assert(vm);
    if (threads_initialized
	&& ((pt = (struct oskit_uvm_per_thread *)
	     pthread_getspecific(oskit_uvm_per_thread_key)) != 0)) {
	vm->vm_proc.p_addr = &pt->pt_user;
	curpcb = &pt->pt_user.u_pcb;
    } else {
	/* Maybe right way (but I'm not sure) */
	vm->vm_proc.p_addr = &oskit_uvm_user;
	curpcb = DEFAULT_PCB;
    }

    oskit_uvm_curvm = vm;
    pmap_activate(&vm->vm_proc);
}
