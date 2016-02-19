/*
 * Copyright (c) 1996, 1998, 1999 University of Utah and the Flux Group.
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

/*
 * Handle the details of stack redzones. For the x86 we must use a task
 * gate to handle stack overflow since faults dump state onto the current
 * stack when there is no priv level change (as is the case in the base
 * oskit). That of course will lead to a double fault, and then a machine
 * reset without any idea of what happened. By installing a task gate for
 * for T_DOUBLE, the double fault will cause a switch into a different task
 * with a new stack. At that point panic gracefully.
 *
 * This is good for single threaded kernels that do not link with the SVM
 * library. This code is part of SVM, and multi-threaded kernels use that
 * to get stack redzones per thread.
 */
#include <oskit/c/malloc.h>
#include <oskit/c/sys/mman.h>
#include <oskit/x86/base_paging.h>
#include <oskit/x86/pc/phys_lmm.h>
#include <oskit/x86/trap.h>
#include <oskit/x86/gate_init.h>
#include <oskit/x86/base_trap.h>
#include <oskit/x86/proc_reg.h>
#include <oskit/x86/base_gdt.h>
#include <oskit/x86/base_idt.h>
#include <oskit/x86/base_tss.h>
#include <oskit/x86/base_stack.h>
#include <oskit/lmm.h>
#include <oskit/dev/dev.h>
#include <stdio.h>
#include <oskit/error.h>
#include <oskit/debug.h>

#define STACKSIZE	(2 * PAGE_SIZE)
static struct x86_tss	task_tss;	/* Our task */

static void		double_fault_handler(void);

/*
 * Setup a ``redzone'' page for the base stack to detect overflows.
 * This routine should be called instead of base_paging_{init,load} as it
 * wants to initialize paging in its own way.
 */
void
setup_redzone(void)
{
	oskit_addr_t	addr;

	if (base_pdir_pa)
		panic("Base paging environment already initialized");
	if (ptab_alloc(&base_pdir_pa))
		panic("Can't allocate kernel page directory"); 

	printf("Mapping 0x%x to 0x%x\n", 0, (int) phys_mem_max);

	addr = (oskit_addr_t) 0;
	while (addr < round_page(phys_mem_max)) {
		if (pdir_map_range(base_pdir_pa, addr, addr, PAGE_SIZE,
			INTEL_PTE_VALID|INTEL_PTE_WRITE|INTEL_PTE_USER))
			panic("setup_redzone: pdir_map_range");
		addr += PAGE_SIZE;
	}

	/*
	 * Map the bottom page of the stack as read-only. It was part of
	 * the data segment, so must unmap it first.
	 */
	pdir_unmap_page(base_pdir_pa, (oskit_addr_t) base_stack_start);

	printf("Mapping 0x%x to 0x%x\n", (int) base_stack_start,
	       (int) base_stack_start + PAGE_SIZE);

	if (pdir_map_range(base_pdir_pa, (oskit_addr_t) base_stack_start,
		(oskit_addr_t) base_stack_start, PAGE_SIZE,
		INTEL_PTE_VALID|INTEL_PTE_USER))
		panic("setup_redzone: Can't set stack redzone");

	/*
	 * Map I/O space. Leave out the stuff at the end to catch references
	 * to -1 and and little bit below that. Use 4MB pages.
	 */
	printf("Mapping 0x%x to 0x%x\n", (int) 0xF0000000, (int) 0XFFFF0000);

	if (pdir_map_range(base_pdir_pa, 0xF0000000, 0xF0000000,
		0XFFFF0000 - 0XF0000000,
		INTEL_PTE_VALID|INTEL_PTE_WRITE|INTEL_PTE_USER))
		panic("setup_redzone: pdir_map_range on I/O space");

	/*
	 * Set the WP bit in CR0. This bit turns on enforcement of read-only
	 * access when a user-mode page is accessed in supervisor mode.
	 */
	set_cr0(get_cr0() | CR0_WP);

	/*
	 * This turns on paging!
	 */
	base_paging_load();

	/*
	 * We use a task gate to catch page faults, since a stack overflow
	 * will try and dump more stuff on the stack. This is the easiest
	 * way to deal with it.
	 */
	if ((addr = (oskit_addr_t) memalign(PAGE_SIZE, STACKSIZE)) == 0)
		panic("redzone_init: Could not allocate stack\n");

	task_tss.ss0   = KERNEL_DS;
	task_tss.esp0  = addr + STACKSIZE - sizeof(double);
	task_tss.esp   = task_tss.esp0;
	task_tss.ss    = KERNEL_DS;
	task_tss.ds    = KERNEL_DS;
	task_tss.es    = KERNEL_DS;
	task_tss.fs    = KERNEL_DS;
	task_tss.gs    = KERNEL_DS;
	task_tss.cs    = KERNEL_CS;
	task_tss.io_bit_map_offset = sizeof(task_tss);
	task_tss.eip    = (int) double_fault_handler;

	/* Make sure the task is started with interrupts disabled */
	osenv_intr_disable();
	task_tss.eflags = (int) get_eflags();
	osenv_intr_enable();
	
	/* Both TSSs has to know about the page tables */
	task_tss.cr3    = get_cr3();
	base_tss.cr3	= get_cr3();

	/* Initialize the base TSS descriptor.  */
	fill_descriptor(&base_gdt[KERNEL_TRAP_TSS / 8],
			kvtolin(&task_tss), sizeof(task_tss) - 1,
			ACC_PL_K|ACC_TSS|ACC_P, 0);

	/*
	 * NOTE: The task switch will include an extra word on the stack,
	 * pushed by the CPU. The handler will need to be in assembly code
	 * if we care about that value. As it is, the handler routine
	 * stack is going to be slightly messed up, but since the handler
	 * calls panic, it is not a problem right now. 
	 */
	fill_gate(&base_idt[T_DOUBLE_FAULT],
		  0, KERNEL_TRAP_TSS, ACC_TASK_GATE|ACC_P|ACC_PL_K, 0);

	base_idt_load();
	base_gdt_load();
}

/*
 * Task entrypoint. A double fault switches into this code. All double
 * faults are fatal, so just print out something useful and panic. It
 * would be nice at some point to hook into the pthreads code so that
 * the current thread information could be dumped too.
 */
static void
double_fault_handler(void)
{
	oskit_addr_t			addr;

	while (1) {
		addr = get_cr2();

		/* This seems to keep netboot from going nuts */
		clear_ts();

		panic("DOUBLE FAULT: "
		      "Maybe its a stack overflow at 0x%x", addr);
	}
}
