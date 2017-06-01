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

#include <oskit/c/stdio.h>
#include <oskit/c/setjmp.h>
#include <oskit/c/errno.h>
#include <oskit/error.h>
#include <oskit/page.h>
#include <oskit/threads/pthread.h>
#include "sproc_internal.h"

pthread_key_t	oskit_sproc_thread_key;
int		oskit_sproc_debug;

extern void
oskit_sproc_init()
{
    pthread_key_create(&oskit_sproc_thread_key, NULL);
    oskit_sproc_machdep_init();
}

extern oskit_error_t
oskit_sproc_create(const struct oskit_sproc_desc *desc, oskit_size_t size,
		   struct oskit_sproc *outproc)
{
    oskit_error_t error;

    error = oskit_uvm_create(size, &outproc->sp_vm);
    if (error) {
	return error;
    }

    queue_init(&outproc->sp_thread_head);
    pthread_mutex_init(&outproc->sp_lock, NULL);

    /* Install our uvm handler */
    oskit_uvm_handler_set(outproc->sp_vm, oskit_sproc_uvm_handler);

    outproc->sp_desc = desc;
    return 0;
}

extern oskit_error_t
oskit_sproc_destroy(struct oskit_sproc *sproc)
{
    oskit_error_t error;
    struct oskit_sproc_thread *sthread;
    int nth = 0;

    pthread_mutex_lock(&sproc->sp_lock);
    queue_iterate(&sproc->sp_thread_head, sthread, struct oskit_sproc_thread *,
		  st_thread_chain) {
	nth++;
    }
    assert(nth == 0);

    error = oskit_uvm_destroy(sproc->sp_vm);
    pthread_mutex_unlock(&sproc->sp_lock);
    pthread_mutex_destroy(&sproc->sp_lock);
    if (error) {
	return error;
    }

    return 0;
}

extern oskit_error_t
oskit_sproc_stack_alloc(struct oskit_sproc *sproc, oskit_addr_t *base,
			oskit_size_t size, oskit_size_t redzonesize, 
			struct oskit_sproc_stack *out_stack)
{
    int rc;
    oskit_addr_t mapaddr;
    oskit_error_t error;

    /* sanity check */
    assert((size & 0x7) == 0);
    assert((redzonesize & (PAGE_SIZE - 1)) == 0);
    assert(redzonesize < size);

    error = oskit_uvm_mmap(sproc->sp_vm, base, size + redzonesize,
			   PROT_READ|PROT_WRITE,
			   MAP_PRIVATE|MAP_ANON|(*base ? MAP_FIXED : 0),
			   0, 0);
    if (error) {
	return error;
    }
    mapaddr = *base;
    if (redzonesize) {
	rc = oskit_uvm_mprotect(sproc->sp_vm, mapaddr, redzonesize, PROT_READ);
	assert(rc == 0);
    }

    out_stack->st_base = mapaddr;
    out_stack->st_size = size;
    out_stack->st_redzonesize = redzonesize;
    /* XXX: downward stack is assumed */
    out_stack->st_sp = mapaddr + redzonesize + size;
    out_stack->st_process = sproc;
    /* does not belong to any thread */
    out_stack->st_thread = 0;

    return 0;
}

extern oskit_error_t
oskit_sproc_stack_push(struct oskit_sproc_stack *stack, void *arg,
		       oskit_size_t argsize)
{
    oskit_vmspace_t ovm;
    oskit_error_t error;

    ovm = oskit_uvm_vmspace_set(stack->st_process->sp_vm);
    error = copyout(arg, (void*)(stack->st_sp - argsize), argsize);
    oskit_uvm_vmspace_set(ovm);
    if (error) {
	return error;
    }
    stack->st_sp -= argsize;
    return 0;
}
