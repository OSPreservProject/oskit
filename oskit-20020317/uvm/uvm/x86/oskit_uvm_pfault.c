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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mman.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/pool.h>
#include <uvm/uvm.h>
#include <sys/user.h>
#include <machine/pcb.h>
#include <machine/trap.h>
#include "opt_cputype.h"

#include <oskit/x86/trap.h>
#include <oskit/x86/base_trap.h>
#include <oskit/x86/gdb.h>
#include <oskit/x86/eflags.h>
#include "oskit_uvm_internal.h"

static void
call_handler(oskit_vmspace_t oskitvm, int signo, struct trap_state *frame)
{
    if (oskitvm->vm_handler) {
	(*oskitvm->vm_handler)(oskitvm, signo, frame);
    } else {
	oskit_sendsig(signo, frame);
    }
}

/*
 * Page fault handler.  Taken from sys/arch/i386/i386/trap.c
 *
 * Note that this handler runs under interrupt enabled.
 * %cr2 is saved/restored when a context switch occurs.
 */
extern int
oskit_uvm_pfault_handler(struct trap_state *frame)
{
    register struct proc *p = curproc;
    oskit_vmspace_t oskitvm = oskit_uvm_curvm;
    struct pcb *pcb = NULL;
    extern char fusubail[];
    int type;

    type = frame->trapno;

    if (!KERNELMODE(frame->cs, frame->eflags)) {
	type |= T_USER;
	p->p_md.md_regs = frame;
    }

    switch (type) {
    case T_PAGE_FAULT:

		if (p == 0)
			goto we_re_toast;
		pcb = &p->p_addr->u_pcb;

		/*
		 * fusubail is used by [fs]uswintr() to prevent page faulting
		 * from inside the profiling interrupt.
		 */
		if (pcb != DEFAULT_PCB && pcb->pcb_onfault == fusubail)
			goto copyfault;
#if 0
		/* XXX - check only applies to 386's and 486's with WP off */
		if (frame->err & PGEX_P)
			goto we_re_toast;
#endif
		/* FALLTHROUGH */

    case T_PAGE_FAULT|T_USER: {	/* page fault */
		register vaddr_t va;
		register struct vmspace *vm;
		register vm_map_t map;
		int rv;
		vm_prot_t ftype;
		extern vm_map_t kernel_map;
		unsigned nss;

		va = trunc_page((vaddr_t)rcr2());
		/*
		 * It is only a kernel address space fault iff:
		 *	1. (type & T_USER) == 0  and
		 *	2. pcb_onfault not set or
		 *	3. pcb_onfault set but supervisor space fault
		 * The last can occur during an exec() copyin where the
		 * argument space is lazy-allocated.
		 */
		if (type == T_PAGE_FAULT && va <= VM_MAX_KERNEL_ADDRESS)
		    map = kernel_map;
		else {
		    vm = p->p_vmspace;
		    assert(vm);
		    map = &vm->vm_map;
		}
		if (frame->err & PGEX_W)
			ftype = VM_PROT_READ | VM_PROT_WRITE;
		else
			ftype = VM_PROT_READ;

#ifdef DIAGNOSTIC
		if (map == kernel_map && va == 0) {
			printf("trap: bad kernel access at %lx\n", va);
			goto we_re_toast;
		}
#endif

		nss = 0;
#if 0
		if ((caddr_t)va >= vm->vm_maxsaddr
		    /*&& (caddr_t)va < (caddr_t)VM_MAXUSER_ADDRESS*/
		    && map != kernel_map) {
			nss = btoc(USRSTACK-(unsigned)va);
			if (nss > btoc(p->p_rlimit[RLIMIT_STACK].rlim_cur)) {
				/*
				 * We used to fail here. However, it may
				 * just have been an mmap()ed page low
				 * in the stack, which is legal. If it
				 * wasn't, uvm_fault() will fail below.
				 *
				 * Set nss to 0, since this case is not
				 * a "stack extension".
				 */
				nss = 0;
			}
		}
#endif

		/* Fault the original page in. */
		UVM_LOCK;
		XPRINTF(OSKIT_DEBUG_FAULT, "CALL uvm_fault, map %p, va 0x%lx, "
			"ftype %x, thread %d, type %d\n",
			map, (long)va, ftype, (int)pthread_self(), type);
		rv = uvm_fault(map, va, 0, ftype);
		XPRINTF(OSKIT_DEBUG_FAULT, "RETURN from uvm_fault, "
			"thread %d, rv %d\n", (int)pthread_self(), rv);
		UVM_UNLOCK;
		if (rv == KERN_SUCCESS) {
#if 0
			if (nss > vm->vm_ssize)
				vm->vm_ssize = nss;
#endif
			if (type == T_PAGE_FAULT)
			    return 0;
			goto out;
		}

		XPRINTF(OSKIT_DEBUG_FAULT, __FUNCTION__
			": uvm_fault failed (%d) accessing va 0x%lx from %s "
			"mode, thread %d\n", rv, (unsigned long)va,
			(type == T_PAGE_FAULT ? "kernel" : "user"),
			(int)pthread_self());

		if (type == T_PAGE_FAULT) {
			if (pcb != DEFAULT_PCB && pcb->pcb_onfault != 0)
				goto copyfault;
			call_handler(oskitvm, SIGSEGV, frame);
			break;
		}
		if (rv == KERN_RESOURCE_SHORTAGE) {
			printf("UVM: proc %p killed: out of swap\n", p);
			if (oskitvm->vm_handler) {
			    (*oskitvm->vm_handler)(oskitvm, SIGKILL, frame);
			}
			oskit_uvm_vmspace_set(&oskit_uvm_kvmspace);
			oskit_uvm_destroy(oskitvm);
			pthread_exit(0);
		} else {
		    	call_handler(oskitvm, SIGSEGV, frame);
		}
		break;
	}
    default:
    we_re_toast:
    	panic(__FUNCTION__"\n");
    }
 out:
    return 0;

 copyfault:
    frame->eip = (int)pcb->pcb_onfault;
    return 0;
}


#ifdef I386_CPU
/*
 * Compensate for 386 brain damage (missing URKR)
 */
int
trapwrite(addr)
	unsigned addr;
{
	vaddr_t va;
	unsigned nss;
	struct proc *p;
	struct vmspace *vm;

	va = trunc_page((vaddr_t)addr);
	if (va < VM_MIN_ADDRESS)
		return 1;

	nss = 0;
	p = curproc;
	vm = p->p_vmspace;
#if 0
	if ((caddr_t)va >= vm->vm_maxsaddr) {
		nss = btoc(USRSTACK-(unsigned)va);
		if (nss > btoc(p->p_rlimit[RLIMIT_STACK].rlim_cur))
			nss = 0;
	}
#endif
	if (uvm_fault(&vm->vm_map, va, 0, VM_PROT_READ | VM_PROT_WRITE)
	    != KERN_SUCCESS)
		return 1;

#if 0
	if (nss > vm->vm_ssize)
		vm->vm_ssize = nss;
#endif

	return 0;
}
#endif
