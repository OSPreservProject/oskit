/*
 * Copyright (c) 1996-2000 University of Utah and the Flux Group.
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
 * Example mini-kernel that illustrates a possible use for SMP.
 */

#include <stdio.h>
#include <oskit/gdb_serial.h>
#include <oskit/x86/multiboot.h>
#include <oskit/x86/base_cpu.h>
#include <oskit/x86/gdb.h>
#include <oskit/x86/pc/reset.h>
#include <oskit/debug.h>
#include <oskit/smp.h>
#include <oskit/machine/spin_lock.h>
#include <oskit/c/malloc.h>
#include <oskit/c/stdlib.h>
#include <oskit/tty.h>
#include <oskit/c/termios.h>
#include <oskit/x86/pc/com_cons.h>
#include <oskit/base_critical.h>
#include <oskit/x86/pc/base_irq.h>	/* for IPIs */
#include <oskit/clientos.h>

volatile int bsp;	/* boot processor number */
volatile int active = 1;
volatile int ipi_count[NR_CPUS];

/*
 * This is the routine that handles an interprocessor interrupt
 * from another CPU.  TLB-shootdowns (if paging was used) can be
 * implemented using this, as can reschedule interrupts (since only
 * the BSP currently receives hardware interrupts).
 */
void
IPI_handler(void)
{
	int cpunum = smp_find_cur_cpu();
	printf("cpu %d received IPI\n", cpunum);
	ipi_count[cpunum]++;

	/* we are done in the handler; ack the interrupt */
	smp_apic_ack();
}

/*
 * This is a sample function for smp_start_cpu()
 * This is the first "OS" C-code that gets executed.
 * It should never return.
 */
void
booted(void *num_proc)
{
	int cpuid = smp_find_cur_cpu();
	printf("in booted: Processor %d; %d processors total!\n",
		cpuid, *(int *)num_proc);

	/* allow receiving IPI */
	smp_message_pass_enable[smp_find_cur_cpu()] = 1;

	/* XXX: relies on increment/decrement being atomic... */
	active++;

	/* send interrupt to the boot processor */
	smp_message_pass(bsp);

	/* wait for IPI */
	while (!ipi_count[cpuid])
		;

	printf("processor %d halting\n", cpuid);
	active--;

	__asm__("hlt");
}

/*
 * Here is the driver to demonstrate the use of the SMP
 */
int
main(int argc, char **argv)
{
	int num_processors;
	int num, curr;

	oskit_clientos_init();

	/* Okay, here we init the SMP code */
	printf("Calling smp_init()...\n");
	if (!smp_init()) {	/* smp init succeeded! */
		if ((num_processors = smp_get_num_cpus()) > 1) {
			printf("smp_init succeeded: ");
		} else {
			printf("Uniprocessor smp-capable system: ");
		}
	} else {	/* non-IMPS-compliant system. */
		printf("smp_init failed: ");
		printf("non-smp system or HAVE_CODE16 not set\n");
		exit(1);
	}

	printf("%d processor", num_processors);
	if (num_processors != 1)
		printf("s");
	printf("\n");

	/*
	 * This initializes the IPI interrupt handler.
	 * If using with FDEV, use the osenv_irq_alloc() function.
	 */
	base_irq_handlers[SMP_IPI_VECTOR] =
		(unsigned int (*)(struct trap_state *))IPI_handler;

	bsp = smp_find_cur_cpu();
	printf("BSP is Processor %d\n", bsp);
	smp_message_pass_enable[bsp] = 1;

	/* Let's find out which processors we have */
	num = 0;
	curr = -1;
	while (num < num_processors)  {
		curr = smp_find_cpu(curr);
		printf("processor %d exists\n", curr);
		assert(curr >= 0);
		if (curr != bsp) {
			/*
			 * Start up the AP's, and have them all
			 * run function booted(&num_processors)
			 */
			smp_start_cpu(curr, booted, &num_processors,
				(void *)malloc(65536)+65536);

		}
		num ++;
	};

	/* Processors are all started. */

	/* wait until receive an IPI from each AP */
	while (ipi_count[bsp] < num_processors - 1)
		;

	/* send an IPI to each AP to have it shutdown */
	num = 0;
	curr = -1;
	while (num < num_processors)  {
		curr = smp_find_cpu(curr);
		if (curr != bsp)
			smp_message_pass(curr);
		num ++;
	};

	printf("bsp sent IPI to each AP\n");

	while (active > 1)
		;
	return 0;
}
