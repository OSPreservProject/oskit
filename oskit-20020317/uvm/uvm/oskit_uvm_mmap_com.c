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

oskit_error_t
oskit_uvm_mmap(struct oskit_vmspace *vm, oskit_addr_t *addr, oskit_size_t size,
	       int prot, int flags, oskit_iunknown_t *iunknown, 
	       oskit_off_t foff)
{
    struct sys_mmap_args a;
    int rc;
    oskit_addr_t raddr;

    SCARG(&a,addr) = (void*)*addr;
    SCARG(&a,len) = (size_t)size;
    SCARG(&a,prot) = prot;
    SCARG(&a,flags) = flags;
    SCARG(&a,fd) = (int)iunknown;
    SCARG(&a,pos) = (off_t)foff;
    UVM_LOCK;
    rc = sys_mmap(&vm->vm_proc, &a, &raddr);
    UVM_UNLOCK;
    *addr = raddr;
    return rc;
}

extern oskit_error_t
oskit_uvm_munmap(struct oskit_vmspace *vm, oskit_addr_t addr, oskit_size_t len)
{
    struct sys_munmap_args a;
    int rc;

    SCARG(&a,addr) = (caddr_t)addr;
    SCARG(&a,len) = (size_t)len;
    UVM_LOCK;
    rc = sys_munmap(&vm->vm_proc, &a, 0);
    UVM_UNLOCK;
    return rc;
}

extern oskit_error_t
oskit_uvm_mprotect(struct oskit_vmspace *vm, oskit_addr_t addr,
		   oskit_size_t len, int prot)
{
    struct sys_mprotect_args a;
    int rc;

    SCARG(&a,addr) = (void*)addr;
    SCARG(&a,len) = len;
    SCARG(&a,prot) = prot;
    UVM_LOCK;
    rc = sys_mprotect(&vm->vm_proc, &a, 0);
    UVM_UNLOCK;
    return rc;
}

extern oskit_error_t
oskit_uvm_madvise(struct oskit_vmspace *vm, oskit_addr_t addr,
		  oskit_size_t len, int behav)
{
    struct sys_madvise_args a;
    int rc;

    SCARG(&a,addr) = (void*)addr;
    SCARG(&a,len) = (size_t)len;
    SCARG(&a,behav) = behav;
    UVM_LOCK;
    rc = sys_madvise(&vm->vm_proc, &a, 0);
    UVM_UNLOCK;
    return rc;
}
