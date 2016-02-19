/*
 * Copyright (c) 1997-2000 University of Utah and the Flux Group.
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
 * Simple test program for trap and interrupt annotations.
 */

#ifndef __ELF__
int main(int argc, char **argv)
{
	printf("No annotation support in a.out\n");
	exit(0);
}
#else

#include <stdio.h>
#include <stdlib.h>
#include <oskit/anno.h>
#include <oskit/x86/base_trap.h>
#include <oskit/x86/base_paging.h>
#include <oskit/x86/pc/phys_lmm.h>
#include <oskit/dev/dev.h>
#include <oskit/clientos.h>
#include <assert.h>

#define WORDS	8
#define HZ	100

void anno_trap_test(void);
void anno_intr_test(void);

int
main(int argc, char **argv)
{
	oskit_clientos_init();

	anno_init();
#if 0
	anno_dump();
#endif
	anno_trap_test();
	anno_intr_test();
	exit(0);
}

/*
 * Use an annotated copy routine to copy memory.
 * If read/write access faults, copy_asm will return non-zero.
 */
void
anno_trap_test(void)
{
	int buf1[WORDS], buf2[WORDS];
	int *badaddr;
	int copy_asm(int *src, int *dst, int nwords);

	printf("\nTrap test...\n");

	/*
	 * Map the kernel and turn on paging so that we can generate a
	 * fault on an invalid memory reference.
	 */
	base_paging_init();
	base_paging_load();
	badaddr = (int *)round_superpage(phys_mem_max);

	printf("Copy valid source to valid dest...");
	if (copy_asm(buf1, buf2, WORDS))
		printf("[FAULT]\n");
	else
		printf("ok\n");

	printf("Copy invalid source to valid dest...");
	if (copy_asm(badaddr, buf2, WORDS))
		printf("caught it\n");
	else
		printf("[NO FAULT]\n");

	printf("Copy valid source to invalid dest...");
	if (copy_asm(buf1, badaddr, WORDS))
		printf("caught it\n");
	else
		printf("[NO FAULT]\n");
}

/*
 * Trap handler for faults in the copy_asm code.
 * val3 contains the address where we should return to indicate an error.
 * This value we stash in the exception state EIP which is restored on
 * return from the fault.
 */
int
copy_recover(struct anno_entry *anno, struct trap_state *tstate)
{
	assert(tstate->eip == anno->val1);

	tstate->eip = anno->val3;
	return 0;
}


/*
 * Lame interrupt test.
 * The loop_asm routine has a loop with lots of memory reference instructions.
 * The interrupt annotation just records where the EIP was at the time of the
 * interrupt.  See, I told you it was lame...
 */
volatile unsigned int loop_intrs;
unsigned int loop_counts[128];
extern int loop_start[], loop_end[];

void
anno_intr_test(void)
{
	int count;
	void loop_asm(void);
	void loop_timer(void);

	if ((unsigned)loop_end - (unsigned)loop_start
	    >= sizeof(loop_counts)/sizeof(int))
		return;

	printf("\nInterrupt test...\n");
	printf("Spin for 10 seconds in [%p-%p]...\n", loop_start, loop_end);
	osenv_timer_init();
	osenv_timer_register(loop_timer, HZ);
	loop_asm();
	osenv_timer_shutdown();
	printf("Took %d interrupts (%d.%02d seconds):\n",
	       loop_intrs, loop_intrs/100, loop_intrs%100);
	for (count = 0; count < sizeof(loop_counts)/sizeof(int); count++)
		if (loop_counts[count])
			printf("%d interrupts at 0x%x\n",
			       loop_counts[count], (unsigned)loop_start+count);

	exit(0);
}

/*
 * Interrupt annotation handler for faults in the loop_asm code.
 * Increment a count for that position in the loop.
 * After ten seconds have elapsed, we set the loop_asm "keep going" flag
 * (EAX) to zero.
 */
void
loop_recover(struct anno_entry *anno, struct trap_state *tstate)
{
	assert(tstate->eip >= anno->val1);
	assert(tstate->eip >= (unsigned)loop_start &&
	       tstate->eip < (unsigned)loop_end);

	if (loop_intrs >= 10*HZ) {
		tstate->eax = 0;
		return;
	}
	loop_counts[tstate->eip - (unsigned)loop_start]++;
}

/*
 * Actual fdev interrupt handler.
 * Increment our "timer."
 */
void
loop_timer(void)
{
	loop_intrs++;
}
#endif /* __ELF__ */
