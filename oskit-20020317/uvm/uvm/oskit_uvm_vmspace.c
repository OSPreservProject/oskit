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

/*
 * For allocating/deallocating vmspace structures
 */

/* kernel's vmspace */
struct oskit_vmspace		oskit_uvm_kvmspace;

/* current vmspace */
oskit_vmspace_t 		oskit_uvm_curvm;

static struct oskit_vmspace	*next_vm;

static oskit_error_t		newvm(oskit_vmspace_t *vmp);
static void			initvm(oskit_vmspace_t vm);

/*
 * Initialize vmspace module. 
 */
extern void
oskit_uvm_vmspace_start()
{
    /* create dummy proc structure for the kernel */
    initvm(&oskit_uvm_kvmspace);
}

/*
 * Just initialize a vmspace structure
 */
static void
initvm(oskit_vmspace_t vm)
{
    int i;
    struct proc *proc;

    memset(vm, 0, sizeof *vm);
    proc = &vm->vm_proc;

    proc->p_addr = 0;
    proc->p_stats = &vm->vm_pstats;
    proc->p_limit = &vm->vm_plimit;

#if 0
    {
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	/* should be recursive */
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&vm->vm_lock, &attr);
	pthread_mutexattr_destroy(&attr);
    }
#endif

    for (i = 0; i < sizeof(proc->p_rlimit)/sizeof(proc->p_rlimit[0]); i++) {
	proc->p_limit->pl_rlimit[i].rlim_cur =
	    proc->p_limit->pl_rlimit[i].rlim_max = RLIM_INFINITY;
    }
}

/*
 * Look for a cached proc structure.  Otherwise, make a new one.
 */
static oskit_error_t
newvm(oskit_vmspace_t *out_vm)
{
    struct oskit_vmspace *vm;
    struct proc *proc;

    UVM_LOCK;
    if ((vm = next_vm) != NULL) {
	next_vm = vm->vm_next;
	proc = &vm->vm_proc;
	*out_vm = vm;
	UVM_UNLOCK;
	return 0;
    }
    UVM_UNLOCK;

    vm = malloc(sizeof *vm, M_PROC, M_WAITOK);
    if (!vm)
	return OSKIT_ENOMEM;

    initvm(vm);
    *out_vm = vm;

    return 0;
}

/*
 * Allocate a oskit_vmspace structure.
 */
extern oskit_error_t
oskit_uvm_vmspace_alloc(oskit_vmspace_t *out_vm)
{
    oskit_error_t ferror;
    oskit_vmspace_t vm;

    if ((ferror = newvm(&vm)) != 0)
	return ferror;

    memset(vm->vm_proc.p_stats, 0, sizeof(struct pstats));

    vm->vm_proc.p_pid = 0;		/* pid is assigned by UVM_LOCK */
    vm->vm_handler = 0;

    *out_vm = vm;

    return 0;
}

/*
 * Return a oskit_vmspace structure to the cache.
 */
extern void
oskit_uvm_vmspace_free(oskit_vmspace_t vm)
{
    UVM_LOCK;
    vm->vm_next = next_vm;
    next_vm = vm;
    UVM_UNLOCK;
}

/*
 * Get current vmspace of current thread
 */
extern oskit_vmspace_t 
oskit_uvm_vmspace_get()
{
    struct oskit_uvm_per_thread *pt;

    if (threads_initialized) {
	pt = pthread_getspecific(oskit_uvm_per_thread_key);
	if (pt && pt->pt_vm) {
	    return pt->pt_vm;
	}
    }

    /* This means, this thread is in-kernel thread */
    return &oskit_uvm_kvmspace;
}

/*
 * Assign a vmspace to the current thread
 */
extern oskit_vmspace_t
oskit_uvm_vmspace_set(oskit_vmspace_t vm)
{
    struct oskit_uvm_per_thread *pt;
    oskit_vmspace_t ovm;

    ovm = oskit_uvm_vmspace_get();

    if (threads_initialized) {
	pt = ((struct oskit_uvm_per_thread*)
	      pthread_getspecific(oskit_uvm_per_thread_key));
	assert(pt);
	pt->pt_vm = vm;
    }

    UVM_LOCK;
    oskit_uvm_activate_locked(vm);
    UVM_UNLOCK;
    
    return ovm;
}

/*
 * Switch to specified thread's vmspace.  This function is intended to be
 * called only from a stack trace function.
 */
extern void
oskit_uvm_switchvm(pthread_t tid)
{
    oskit_vmspace_t vm = 0;
    struct oskit_uvm_per_thread *pt;

    if (threads_initialized) {
	pt = ((struct oskit_uvm_per_thread*)
	      oskit_pthread_getspecific(tid, oskit_uvm_per_thread_key));
	if (pt) {
	    vm = pt->pt_vm;
	}
    }
    if (vm == 0) {
	/* This means, this thread is in-kernel thread */
	vm = &oskit_uvm_kvmspace;
    }

    /* Change the vmspace */
    oskit_uvm_vmspace_set(vm);
}

extern void
oskit_uvm_handler_set(oskit_vmspace_t vm, oskit_uvm_handler handler)
{
    vm->vm_handler = handler;
}

extern oskit_uvm_handler
oskit_uvm_handler_get(oskit_vmspace_t vm)
{
    return vm->vm_handler;
}
