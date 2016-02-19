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

static int sendsig_hook(int signo, int code, struct trap_state *ts);

extern void
oskit_sproc_machdep_init()
{
    /* Setup the system call trap vector */
    gate_init(base_idt, oskit_sproc_svc_trap_inittab, KERNEL_CS);

    /* Setup GDTs */
    fill_descriptor(&base_gdt[USER_CS >> 3],
		    0x00000000, 0xffffffff,
		    ACC_PL_U | ACC_CODE_R, SZ_32);
    fill_descriptor(&base_gdt[USER_DS >> 3],
		    0x00000000, 0xffffffff,
		    ACC_PL_U | ACC_DATA_W, SZ_32);

    /* Setup the context switch hook */
    oskit_uvm_csw_hook_set(oskit_sproc_csw);

    /* Install our signal handler hook */
    {
	typedef int	(*sendsig_hook_t)(int sig, int code, 
					  struct trap_state *ts);
	extern sendsig_hook_t
	    oskit_sendsig_hook_set(sendsig_hook_t hook);

	oskit_sendsig_hook_set(sendsig_hook);
    }
}

static int
sendsig_hook(int signo, int code, struct trap_state *ts)
{
    struct oskit_sproc_thread *sthread;
    int rc;

    sthread =
	(struct oskit_sproc_thread*)pthread_getspecific(oskit_sproc_thread_key);

    /* 
     * Call the handler if any.  If the handler returns non zero,
     * rewind the stack to oskit_sproc_switch() to terminate the thread.
     */
    if (sthread && sthread->st_process->sp_desc->sd_handler) {
	rc = (*sthread->st_process->sp_desc->sd_handler)(sthread,
							 signo, code, ts);
	if (rc) {
	    OSKIT_SPROC_RETURN(sthread, rc);
	    /*NOTREACHED*/
	}
	/* Not to deliver the signal, return 0.  See kern/x86/sendsig.c */
	return 0;
    }

    /* Handler is not installed.  Do normal signal delivery. */
    return 1;
}

#if 1
extern void
dump_segregs()
{
    printf("Current Segment Registers:\n");
    printf("CS %04x SS %04x DS %04x ES %04x FS %04x GS %04x\n\n",
	   get_cs() , get_ss(),
	   get_ds(), get_es(), get_fs(), get_gs());
}
#endif
