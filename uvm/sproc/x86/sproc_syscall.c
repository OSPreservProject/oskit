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

#include <oskit/c/setjmp.h>
#include <oskit/c/stdio.h>
#include <oskit/debug.h>
#include <oskit/dev/dev.h>
#include <oskit/error.h>
#include <oskit/threads/pthread.h>
#include <oskit/x86/eflags.h>
#include <oskit/x86/trap.h>
#include "sproc_internal.h"

/*
 * System call handler, called from svc_trap_inittab.
 */
extern int
oskit_sproc_syscall_handler(struct trap_state *ts)
{
    unsigned int syscallno;
    int *arg_kv;
    struct oskit_sproc *sproc;
    struct oskit_sproc_thread *sthread;
    int rval[2];
    int args[MAXSYSCALLARGS];
    int error;
    struct oskit_sproc_sysent *syscalltab;
    oskit_sproc_syscall_func func;

    /*
     * Make sure the interruption is enabled.
     * Since this function is called through trap gate, the CPU's 
     * interrupt flag must be cleared at this point.  However, 
     * osenv_intr_enabled() might return false if linked with with the OSKit
     * realtime library instead of the device library because the realtime 
     * library manage their interruption state separately with the CPU's 
     * interruption flag.
     */
    osenv_intr_enable();

    /* Get the current executing process */
    sthread = 
	(struct oskit_sproc_thread*)pthread_getspecific(oskit_sproc_thread_key);
    sproc = sthread->st_process;

    /* Get the system call number */
    syscallno = ts->eax;
    syscalltab = &sproc->sp_desc->sd_syscalltab[syscallno];

#if 0
    printf(__FUNCTION__": Enter system call handler (trapno = %d)\n",
	    ts->trapno);
    printf("thread %d, stack = 0x%x\n", (int)pthread_self(), get_esp());
#endif

    if (sproc->sp_desc->sd_nsyscall <= syscallno
	|| (func = syscalltab->entry) == 0) {
	printf(__FUNCTION__": Undefined System Call (%d). Ignored\n",
	       syscallno);
	error = OSKIT_ENOSYS;
	goto bad;
    }

    /* And the pointer to the first argument */
    arg_kv = (int*)(ts->esp + 4);

    /* Copy arguments */
    assert(syscalltab->nargs <= MAXSYSCALLARGS);
    error = copyin(arg_kv, args, syscalltab->nargs * sizeof(int));
    if (error) {
	goto bad;
    }

    rval[0] = rval[1] = 0;

    /* Dispatch the system call */
    error = (*func)(sthread, (void*)arg_kv, rval);

    switch (error) {
    case 0:
	/* Store the return value */
	ts->eax = rval[0];
	ts->edx = rval[1];
	ts->eflags &= ~EFL_CF;	/* carry bit */
	break;

    default:
    bad:
	ts->eax = error;
	ts->eflags |= EFL_CF;	/* carry bit */
	break;
    }

    /* Should return 0 */
    return 0;
}

