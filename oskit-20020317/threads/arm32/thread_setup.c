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
 * Machine dependent thread initialization.
 */
#include <threads/pthread_internal.h>
#include <oskit/c/sys/mman.h>
#include <oskit/machine/trap.h>
#include <oskit/machine/base_trap.h>
#include <oskit/machine/proc_reg.h>

extern int		oskit_usermode_simulation;
extern void		thread_switch_realmode();
void			(*thread_switch)(pthread_thread_t *pnext,
				pthread_lock_t *l, pthread_thread_t *cur) = 0;

/*
 * Machine dependent pthread system initialization.
 */
void
thread_machdep_init()
{
	if (oskit_usermode_simulation) {
		panic("thread_machdep_init: No prepared for user mode yet");
	}
	else {
		thread_switch = thread_switch_realmode;
	}
}

/*
 * Set up an initial context. Arguments are passed in registers, and the
 * the return address is taken from the link register in the PCB, so it
 * kinda easy.
 *
 * The starter routine takes the new pthread as its argument. It does
 * some intitialization stuff, and then applies the user specified start
 * function(argument). 
 */
extern void	pthread_start_thread(void);

void
thread_setup(pthread_thread_t *pthread)
{
	pcb_t		*ppcb = pthread->ppcb;
	oskit_u32_t	*pframe;

#ifdef	STACKGUARD
	if (pthread->guardsize)
		pthread_mprotect((oskit_addr_t) pthread->pstk,
				 pthread->guardsize, PROT_READ);
#endif
	/*
	 * The initial frame starts at the other end of the stack
	 * memory. 
	 */
	pframe    = (oskit_u32_t *) (((oskit_u32_t) pthread->pstk) +
				     pthread->ssize);

	ppcb->r14 = (oskit_u32_t) pthread_start_thread; /* lr */
	ppcb->r13 = (oskit_u32_t) pframe;		/* sp */

	/*
	 * Note: The new thread pointer is passed to the start routine
	 * by virtue of the fact that r0 is preserved across the context
	 * switch, and happens to be the new thread pointer! Yippie!
	 */
}

/*
 * Machine dependent thread destroy.
 */
void
thread_destroy(pthread_thread_t *pthread)
{
#ifdef	STACKGUARD
	if (pthread->guardsize)
		pthread_mprotect((oskit_addr_t) pthread->pstk,
				 pthread->guardsize, PROT_READ|PROT_WRITE);
#endif
}

/*
 * Dump a stack backtrace for a switched out thread.
 */
int
threads_stack_back_trace(int tid, int max_st_levels)
{
        unsigned int *fp, i;
	pthread_thread_t	*pthread;

	if ((pthread = tidtothread((pthread_t) tid)) == NULL_THREADPTR)
		return EINVAL;

	if (pthread == CURPTHREAD())
		return EINVAL;

	fp = (unsigned int *) pthread->pcb.r11;

        printf("Backtrace tid:%d P:%08x: lr:0x%08x fp:0x%08x\n\t", tid,
	       (int) pthread, (int) pthread->pcb.r14, (int) fp);
	for (i = 0; i < max_st_levels; i++) {
		if ((i % 8) == 0)
			printf("\n");
			
		fp = (int *)(*(fp - 3));
		
		if (fp == 0)			
			break;

		printf(" %08x", *(fp - 1));
	}
        printf("\n");

	return 0;
}

/*
 * Get machine-specific thread state and stash into the given pointer.
 * This is used by pthread_getstate.
 */
void
thread_getstate(pthread_thread_t *pthread, pthread_state_t *pstate)
{
	pstate->stacksize = pthread->ssize - pthread->guardsize;
	pstate->stackbase = (ssize_t)pthread->pstk + pthread->guardsize;

	if (pthread == CURPTHREAD()) {
		pstate->stackptr  = (oskit_u32_t) get_sp();
		pstate->frameptr  = (oskit_u32_t) get_fp();
	}
	else {
		pstate->stackptr  = (oskit_u32_t) pthread->pcb.r13;
		pstate->frameptr  = (oskit_u32_t) pthread->pcb.r11;
	}
}
