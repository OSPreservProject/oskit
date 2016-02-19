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

/* kernel vmspace */
struct vmspace	vmspace0;
int		oskit_uvm_initialized = 0;

/* To make sure to link with UVM's memory allocator */
extern int	oskit_mem_uvm;
int		*oskit_uvm_force_link_oskit_mem_uvm = &oskit_mem_uvm;

extern void
oskit_uvm_init()
{
    /* UVM code depends on POSIX threads */
    if (!threads_initialized || oskit_uvm_initialized) {
	return;
    }

    /* Sanity check */
    assert (VM_PROT_NONE == PROT_NONE
	    && VM_PROT_READ == PROT_READ
	    && VM_PROT_WRITE == PROT_WRITE
	    && VM_PROT_EXECUTE == PROT_EXEC);

    /*
     * Set up oskit_uvm_kvmspace.
     */
    oskit_uvm_vmspace_start();
    
    oskit_uvm_curvm = &oskit_uvm_kvmspace;

    /*
     * MD initialization
     */
    oskit_uvm_machdep_init();

    /*
     * Init imask used by splXXX.  Theese values should be not zero.
     */
    {
	int i;
	for (i = 0 ; i < NIPL ; i++) {
	    imask[i] = 1 << i;
	}
    }

    /*
     * Initialize UVM!
     */
    uvm_init();

    /* vmspace0 has already initialized in uvm_km_init(). */
    curproc->p_vmspace = &vmspace0;

    /* Should be earlier than calling vmspace_set. */
    oskit_uvm_thread_init();

    /* Set this thread's vmspace. */
    oskit_uvm_vmspace_set(&oskit_uvm_kvmspace);

    /* XXX */
    UVMHIST_INIT(ubchist, 300);

    /* Setup the vmswitch hook for panic() */
    {
	extern void (*oskit_vm_switch)(pthread_t);
	oskit_vm_switch = oskit_uvm_switchvm;
    }

    /* Initialize done! */
    oskit_uvm_initialized = 1;

    /* Do mprotect() reqeusted before uvm is initialized. */
    oskit_uvm_do_pending_mprotect();
}
