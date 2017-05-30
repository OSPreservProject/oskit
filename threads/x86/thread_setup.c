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

/*
 * Pointer to fpustate for the thread that last used the FP unit. This
 * points into a PCB structure, and starts out pointing at the main
 * thread. At each context switch the TS flag is set. An attempt to
 * use the FPU will cause an NM exception. The current state of the
 * FPU will be saved into *cur_fpustate, cur_fpustate reset to point
 * at the current thread PCB, and an FP reload done. The TS flag is
 * cleared so that the FP operation can continue.
 */
pthread_thread_t	*owner_fpu;
extern pthread_thread_t	threads_mainthread;
extern int		pthread_fpu_trap_handler(struct trap_state *ts);
extern int		oskit_usermode_simulation;
extern void		thread_switch_usermode(), thread_switch_realmode();
void			(*thread_switch)(pthread_thread_t *pnext,
				pthread_lock_t *l, pthread_thread_t *cur) = 0;
void			(*oskit_vm_switch)(pthread_t);

/*
 * Machine dependent pthread system initialization.
 */
void
thread_machdep_init()
{
	if (! oskit_usermode_simulation) {
#ifndef SMP
		/*
		 * Override NM trap handler.
		 */
		base_trap_handlers[T_NO_FPU] = pthread_fpu_trap_handler;

		owner_fpu     = &threads_mainthread;
#endif
		thread_switch = thread_switch_realmode;
	}
	else {
		thread_switch = thread_switch_usermode;
	}
}

/*
 * Set up an initial stack that looks like:
 *
 *       ------------
 *      |  Pthread   |
 *       ------------
 *      |  Panic RA  |
 *       ------------
 *      |   Starter  |
 *       ------------  <--- SP
 *
 * The starter routine takes the new pthread as its argument. It does
 * some intitialization stuff, and then applies the user specified start
 * function(argument). Set up a panic just in case something goes wrong
 * and the threads unwinds too far.
 */
extern void	pthread_start_thread(void);
extern void	thread_fpuinit(oskit_u32_t *, oskit_u32_t *);

void
thread_setup(pthread_thread_t *pthread)
{
	pcb_t		*ppcb = pthread->ppcb;
	oskit_u32_t	*pframe;
	void		thread_bottom_exit();
	oskit_u32_t	fstate[32];

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
	pframe   -= 3;
	pframe[0] = (oskit_u32_t) pthread_start_thread;
	pframe[1] = (oskit_u32_t) thread_bottom_exit;
	pframe[2] = (oskit_u32_t) pthread;

	/*
	 * Setting the EIP would seem redundant, but the context switch
	 * code is going to use it instead of the location on the stack.
	 */
	ppcb->esp = (oskit_u32_t) pframe;
	ppcb->eip = (oskit_u32_t) pthread_start_thread;

	/*
	 * Set up the current floating point state. Done in asm code.
	 */
	thread_fpuinit(fstate, pthread->pcb.fstate);
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
	if (owner_fpu == pthread)
		owner_fpu = &threads_mainthread;
}

void
thread_bottom_exit()
{
	panic("thread_bottom_exit:");
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

	fp = (unsigned int *) pthread->pcb.ebp;

        printf("Backtrace tid:%d P:%08x: eip:0x%08x fp:0x%08x\n\t", tid,
	       (int) pthread, (int) pthread->pcb.eip, (int) fp);

	/* Switch the vmspace */
	if (oskit_vm_switch) {
	    (*oskit_vm_switch)((pthread_t)tid);
	}

        for (i = 0; i < max_st_levels && fp != NULL; i++) {
                if (!(*(fp + 1) && *fp))
                        break;

                printf(" %08x", *(fp + 1));

                fp = (int *)(*fp);
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
		pstate->stackptr  = (oskit_u32_t) get_esp();
		pstate->frameptr  = (oskit_u32_t) get_ebp();
	}
	else {
		pstate->stackptr  = (oskit_u32_t) pthread->pcb.esp;
		pstate->frameptr  = (oskit_u32_t) pthread->pcb.ebp;
	}
}
