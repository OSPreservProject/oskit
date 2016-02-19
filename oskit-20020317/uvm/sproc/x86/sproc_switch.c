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

#include "sproc_machdep.h"

/*
 *  Transfer control to the user address space
 */
extern void
oskit_sproc_switch(struct oskit_sproc *sproc, 
		   oskit_addr_t entry, struct oskit_sproc_stack *stack)
{
    int ret;
    int intr_enabled;
    struct oskit_sproc_thread sthread;

    assert(stack->st_process == sproc);

    XPRINTF(OSKIT_DEBUG_SWITCH, 
	    __FUNCTION__": thread %d, activate the vmspace\n",
	    (int)pthread_self());

    pthread_mutex_lock(&sproc->sp_lock);

    /*
     * Register to the process's thread list
     */
    queue_enter(&sproc->sp_thread_head, &sthread, struct oskit_sproc_thread *,
		st_thread_chain);

    pthread_mutex_unlock(&sproc->sp_lock);

    sthread.st_process = sproc;
    sthread.st_stack = stack;
    /* make sure the stack is not used */
    assert(stack->st_thread == 0);
    stack->st_thread = &sthread;

    /*
     * Initialize the tss.  This is used for all traps during executing
     * the user code.
     */
    sthread.st_machdep.x86_tss = base_tss;
    sthread.st_machdep.x86_tss.esp0 = get_esp();

    /* sthread.st_machdep.x86_tss_desc is a kind of cache to allow us not to 
     * call fill_descriptor every time */
    fill_descriptor(&sthread.st_machdep.x86_tss_desc,
		    kvtolin(&sthread.st_machdep.x86_tss), 
		    sizeof(sthread.st_machdep.x86_tss) - 1,
		    ACC_PL_K | ACC_TSS | ACC_P, 0);

    /* Bind this thread to the oskit_sproc_thread structure. */
    pthread_setspecific(oskit_sproc_thread_key, (void*)&sthread);

    /* Assign and activate the vmspace. */
    oskit_uvm_vmspace_set(sproc->sp_vm);

    intr_enabled = osenv_intr_save_disable();

    UPDATE_TSS(&sthread);

    if (intr_enabled) {
	osenv_intr_enable();
    }

    /* push fake return address */
    stack->st_sp -= 4;

    if ((ret = setjmp(sthread.st_context)) == 0) {

	/* Reset segment registers */
	set_fs(0);
	set_gs(0);

	/* taken from 386BSD */
	asm volatile (
	      "		     
              # build outer stack frame
              pushl   %1                      # user stack segment.
              pushl   %2                      # user stack pointer.
              pushl   %0                      # user code segment.
              pushl   %3                      # user code entry point.
              movl    %1,%%eax
              movl    %%eax,%%ds              # ds = ss
              movl    %%eax,%%es              # es = ss
              movl    $0,%%ebp                # initial frame pointer.

              lret                            # go user!
	" :: 
	      "g"(USER_CS),
	      "g"(USER_DS),
	      "g"(stack->st_sp),
	      "g"(entry));
	/* NOTRETURN */
    }

    /*
     * Returned from user mode
     */
    intr_enabled = osenv_intr_save_disable();

    /*
     * Return to the kernel vmspace
     */
    oskit_uvm_vmspace_set(&oskit_uvm_kvmspace);

    /*
     * Restore the original tss
     */
    fill_descriptor(&base_gdt[BASE_TSS / 8],
		    kvtolin(&base_tss), sizeof(base_tss) - 1,
		    ACC_PL_K | ACC_TSS | ACC_P, 0);
    set_tr(BASE_TSS);

    pthread_setspecific(oskit_sproc_thread_key, 0);

    pthread_mutex_lock(&sproc->sp_lock);
    queue_remove(&sproc->sp_thread_head, &sthread, struct oskit_sproc_thread *,
		 st_thread_chain);
    pthread_mutex_unlock(&sproc->sp_lock);

    if (intr_enabled) {
	osenv_intr_enable();
    }
}

