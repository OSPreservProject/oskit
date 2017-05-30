/*
 * Copyright (c) 1996, 1998, 1999, 2001 University of Utah and the Flux Group.
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

/* This file is taken from svm/x86/svm_redzone.c */

/*
 * Handle the details of stack redzones. For the x86 we must use a task
 * gate to handle stack overflow since faults dump state onto the current
 * stack when there is no priv level change (as is the case in the base
 * oskit). That of course will lead to a double fault, and then a machine
 * reset without any idea of what happened. By installing a task gate for
 * for T_DOUBLE, the double fault will cause a switch into a different task
 * with a new stack. At that point we can figure out what thread caused the
 * stack overflow, and then shutdown gracefully. It would be nice if we
 * could recover and just kill that thread, but a double fault is deemed
 * non-recoverable, and appears to leave the machine in an odd state.
 */
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
#include <oskit/lmm.h>
#include <oskit/dev/dev.h>
#include <oskit/debug.h>
#include <oskit/c/stdio.h>

#define STACKSIZE	(2 * PAGE_SIZE)
static struct x86_tss	task_tss;	/* Our task */

extern lmm_t	malloc_lmm;
void		double_fault_handler(void);

/*
 * Init the VM code. 
 */
void
oskit_uvm_redzone_init(void)
{
	oskit_addr_t	addr;

	/*
	 * We use a task gate to catch page faults, since a stack overflow
	 * will try and dump more stuff on the stack. This is the easiest
	 * way to deal with it.
	 */
	if ((addr = (oskit_addr_t)
	            lmm_alloc_aligned(&malloc_lmm, STACKSIZE, 0, 12, 0)) == 0)
		panic(__FUNCTION__": Could not allocate stack\n");

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
void
double_fault_handler(void)
{
	oskit_addr_t addr;
	struct x86_desc *sd;
	extern void tss_dump(struct x86_tss *);

	while (1) {
		addr = get_cr2();

		/* This seems to keep netboot from going nuts */
		clear_ts();

		printf("\nDOUBLE FAULT: possible stack overflow at %#x\n",
		       addr);

		/*
		 * Read the back_link field of the current TSS and use that
		 * to index into the GDT and obtain the previous TSS segment
		 * descriptor.  Use the base fields of that descriptor to get
		 * the linear address of the x86_tss struct.  Finally, pass
		 * lintokv of that address to tss_dump to get useful info!
		 */
		sd = &base_gdt[task_tss.back_link / 8];
		addr = ((sd->base_high << 24) | (sd->base_med << 16) |
			sd->base_low);
		tss_dump((struct x86_tss *)lintokv(addr));
		tss_dump_stack_trace((struct x86_tss *)lintokv(addr), 64);

		panic("DOUBLE FAULT panic");
	}
}
