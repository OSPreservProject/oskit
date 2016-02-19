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

/* This file is included from NetBSD's <sys/proc.h> */

#ifndef _OSKIT_UVM_VMSPACE_H_
#define _OSKIT_UVM_VMSPACE_H_
			  
#include <sys/user.h>		/* struct user */
#include <sys/resourcevar.h>	/* struct plimit */

#include <oskit/uvm.h>

/*
 * This structure represents a virtual memory space in the OSKit UVM.
 * Contains the fake process and user structure used by the NetBSD code.
 *
 * Should be protected by UVM_LOCK/UVM_UNLOCK.
 */
struct oskit_vmspace {
    struct proc			vm_proc;
    struct pstats		vm_pstats;
    struct plimit		vm_plimit;
    struct oskit_vmspace	*vm_next;
    oskit_uvm_handler		vm_handler;
#if 0
    pthread_mutex_t		vm_lock;
#endif
};

/* kernel vmspace */
extern struct oskit_vmspace	oskit_uvm_kvmspace;

/* current vmspace */
extern oskit_vmspace_t		oskit_uvm_curvm;

#define proc0 (oskit_uvm_kvmspace.vm_proc)
#define curproc (&oskit_uvm_curvm->vm_proc)

#endif /*_OSKIT_UVM_VMSPACE_H_*/
