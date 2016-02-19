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

/* for debugging */
int	oskit_uvm_debug;

/*
 * API: Create a vmspace
 */
extern oskit_error_t
oskit_uvm_create(oskit_size_t size, oskit_vmspace_t *out_vm)
{
    oskit_vmspace_t p;
    oskit_error_t error;

    if (VM_MIN_ADDRESS + size > VM_MAXUSER_ADDRESS) {
	return OSKIT_EINVAL;
    }

    error = oskit_uvm_vmspace_alloc(&p);
    if (error) {
	return error;
    }

    UVM_LOCK;
    p->vm_proc.p_vmspace = uvmspace_alloc(VM_MIN_ADDRESS, 
					  VM_MIN_ADDRESS + size, 1);
    UVM_UNLOCK;
    *out_vm = p;

    return 0;
}

/*
 * API: Destroy a vmspace
 */
extern oskit_error_t
oskit_uvm_destroy(oskit_vmspace_t vm)
{
    assert(vm != &oskit_uvm_kvmspace);
    assert(&vm->vm_proc != curproc);

    UVM_LOCK;
    uvmspace_free(vm->vm_proc.p_vmspace);
    UVM_UNLOCK;
    oskit_uvm_vmspace_free(vm);

    return 0;
}
