/*
 * Copyright (c) 1996-1999 The University of Utah and the Flux Group.
 * 
 * This file is part of the OSKit SMP Support Library, which is free software,
 * also known as "open source;" you can redistribute it and/or modify it under
 * the terms of the GNU General Public License (GPL), version 2, as published
 * by the Free Software Foundation (FSF).
 * 
 * The OSKit is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GPL for more details.  You should have
 * received a copy of the GPL along with the OSKit; see the file COPYING.  If
 * not, write to the FSF, 59 Temple Place #330, Boston, MA 02111-1307, USA.
 */

#include <oskit/config.h>
#include <oskit/smp.h>
#include <oskit/x86/smp.h>
#include <oskit/base_critical.h>
#include "linux-smp.h"
#include <oskit/x86/multiboot.h>
#include <stdio.h>
#include <assert.h>
#include <oskit/x86/proc_reg.h>
#include <oskit/x86/base_paging.h>
#include <oskit/lmm.h>
#include <oskit/x86/pc/phys_lmm.h>

#ifdef HAVE_CODE16 /* SMP startup requires 16-bit assembly code support */

/* these are from linux_smp.c */
int smp_scan_config(unsigned long base, unsigned long length);
int smp_boot_cpus();
extern unsigned int num_processors;
extern unsigned char *kernel_stacks[];
extern unsigned long cpu_present_map;

/* from boot.S */
extern long _SMP_TRAMP_START_;

extern struct multiboot_info boot_info;

#define SMP_PRINTK(args) /* printf args */

extern unsigned long apic_addr;
extern unsigned long io_apic_addr;


int smp_get_num_cpus()
{
	static int ret=-1;

	if (ret==-1) {
		SMP_PRINTK(("Getting number of CPU's\n"));

		/* Scan the bottom 1K for a signature */
		ret = smp_scan_config(0x0,0x400);
		/* returns 0 on fail, 1 on success */

		/* Scan the top 1K of base RAM */
		/* get the top of memory -- size from the multi-boot stuff!!! */
		if (boot_info.mem_lower!=640)
			printf("Warning: mem_lower is %dk\n",
				boot_info.mem_lower);
		if (!ret)
			ret=smp_scan_config((boot_info.mem_lower-1)*0x400,0x400);

		/* Scan the 64K of bios */
		if (!ret)
			ret=smp_scan_config(0xF0000,0x10000);

	/* XXX -- get this from Erich Boleyn's patches */
	/*
	 *	If it is an SMP machine we should know now, unless the configuration
	 *	is in an EISA/MCA bus machine with an extended bios data area. I don't
	 *	have such a machine so someone else can fill in the check of the EBDA
	 *	here.
	 */


		if (num_processors == 1)
			cpu_present_map = 1 << smp_find_cur_cpu();
	}
	return num_processors;
}


int smp_init()
{
	int num_cpus;
	int ret;

	/* XXX hack to make sure linker gets SMP versions of these */
	base_critical_enter();
	base_critical_leave();

	SMP_PRINTK(("in SMP_INITIALIZE\n"));

	num_cpus = smp_get_num_cpus();
	/*
	 * Okay.  By now, the SMP config info has been read.
	 * Now all I need to do is startup the APs and get them
	 * spinning until I'm ready for them later.
	 */

	ret = smp_boot_cpus();
	/* this should leave the APs in a loop spinning on a variable */
	SMP_PRINTK(("back from smp_boot_cpus!\n"));

	return ret;
}


extern unsigned long smp_boot_func;
extern unsigned long smp_boot_data;
extern unsigned long smp_boot_stack;
extern unsigned long smp_boot_spin;
extern unsigned long smp_boot_ack;

void smp_start_cpu(int processor_id, void (*func)(void *data),
		void *data, void *stack_ptr)
{
	volatile unsigned long *ack;

	SMP_PRINTK(("In SMP_START_CPU\n"));

	SMP_PRINTK(("APIC %d, func @%p, data @%p, stack @%p\n", processor_id,
		func, data, stack_ptr));

	/* set up the data in the tramp space for the processor */
	assert(kernel_stacks[processor_id]);

	/* should put data on the stack in C code...I'm doing it in assembly */

	/* XXX this is really ugly code */
	*(long *)((long)kernel_stacks[processor_id] +
		((long)&smp_boot_func - (long)&_SMP_TRAMP_START_)) = (long)func;
	*(long *)((long)kernel_stacks[processor_id] +
		((long)&smp_boot_data - (long)&_SMP_TRAMP_START_)) = (long)data;
	*(long *)((long)kernel_stacks[processor_id] +
		((long)&smp_boot_stack - (long)&_SMP_TRAMP_START_)) =
			(long)stack_ptr;

	/*
	 * The processor will set this word to a nonzero value
	 * when it is no longer using the trampoline page.
	 */
	ack = (void *)((long)kernel_stacks[processor_id] +
		       ((long)&smp_boot_ack - (long)&_SMP_TRAMP_START_));
	*ack = 0;

	/*
	 * The processor is spinning as long as this flag is zero.
	 * Set the flag so he will start going.
	 */
	*(long *)((long)kernel_stacks[processor_id] +
		((long)&smp_boot_spin - (long)&_SMP_TRAMP_START_)) = 1;

	/*
	 * Spin until the processor sets the ack word to signal
	 * us it's no longer using the trampoline page, then free it.
	 */
	while (*ack == 0);	/* busy wait */

	lmm_free_page(&malloc_lmm, kernel_stacks[processor_id]);


	SMP_PRINTK(("Leaving SMP_START_CPU\n"));
}




int smp_find_cur_cpu(void)
{
	return smp_processor_id();
}


/*
 * The 32 limit is due to the bit storage limit (from Linux)
 */
int smp_find_cpu(int this)
{
	int next;

	next = this + 1;

	while ((next < 32) && (!(cpu_present_map & (1<<next))))
		next++;

	if (next == 32) next = E_SMP_NO_PROC;
	return next;
}



/*
 * This maps the required regions for paging.
 */
int smp_init_paging(void)
{
	apic_addr = smp_map_range(apic_addr, APIC_SIZE);
	printf("apic at %p\n", (void*)apic_addr);
	io_apic_addr = smp_map_range(io_apic_addr, APIC_SIZE);
	printf("io_apic at %p\n", (void*)io_apic_addr);

	return 0;
}



#else /* not HAVE_CODE16 */

int smp_init() { return -1; /*XXX*/ }
int smp_get_num_cpus() { assert(0); }
int smp_find_cur_cpu(void) { assert(0); }
int smp_find_cpu(int this) { assert(0); }
void smp_start_cpu(int processor_id, void (*func)(void *data),
		void *data, void *stack_ptr) { assert(0); }
int smp_init_paging(void) {assert(0); }

#endif /* HAVE_CODE16 */
