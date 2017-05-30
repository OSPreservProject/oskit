/*
 * Copyright (c) 1997-1998 University of Utah and the Flux Group.
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
/*-
 * Copyright (c) 1982, 1986, 1990, 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)kern_synch.c	8.6 (Berkeley) 1/21/94
 * $\Id: kern_synch.c,v 1.11.4.1 1996/01/31 13:12:29 davidg Exp $
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/buf.h>
#include <sys/signalvar.h>
#include <sys/resourcevar.h>
#include <sys/signalvar.h>
#include <vm/vm.h>
#include "glue.h"

#include <machine/cpu.h>

int	lbolt;			/* once a second sleep address */

void	endtsleep __P((void *));

#ifdef OSKIT
void
#else
static void
#endif
schedcpu(arg)
	void *arg;
{
	wakeup((caddr_t)&lbolt);
	timeout(schedcpu, (void *)0, hz);
}

/*
 * We're only looking at 7 bits of the address; everything is
 * aligned to 4, lots of things are aligned to greater powers
 * of 2.  Shift right by 8, i.e. drop the bottom 256 worth.
 */
#define TABLESIZE	128
static TAILQ_HEAD(slpquehead, proc) slpque[TABLESIZE];
#define LOOKUP(x)	(((intptr_t)(x) >> 8) & (TABLESIZE - 1))

/*
 * During autoconfiguration or after a panic, a sleep will simply
 * lower the priority briefly to allow interrupts, then return.
 * The priority to be used (safepri) is machine-dependent, thus this
 * value is initialized and maintained in the machine-dependent layers.
 * This priority will typically be 0, or the lowest priority
 * that is safe for use on the interrupt stack; it can be made
 * higher to block network software interrupts after panics.
 */
int safepri;

/*
 * General sleep call.  Suspends the current process until a wakeup is
 * performed on the specified identifier.  The process will then be made
 * runnable with the specified priority.  Sleeps at most timo/hz seconds
 * (0 means no timeout).  If pri includes PCATCH flag, signals are checked
 * before and after sleeping, else signals are not checked.  Returns 0 if
 * awakened, EWOULDBLOCK if the timeout expires.  If PCATCH is set and a
 * signal needs to be delivered, ERESTART is returned if the current system
 * call should be restarted if possible, and EINTR is returned if the system
 * call should be interrupted by the signal (return EINTR).
 */
int
tsleep(ident, priority, wmesg, timo)
	void *ident;
	int priority, timo;
	const char *wmesg;
{
	struct proc *p = curproc;
	int s, catch = priority & PCATCH;
	struct callout_handle thandle;
	int interrupted;

	s = splhigh();
	KASSERT(p != NULL, ("tsleep1"));
        KASSERT(ident != NULL && p->p_stat == SRUN, ("tsleep"));
        /*
         * Process may be sitting on a slpque if asleep() was called, remove
         * it before re-adding.
         */
        if (p->p_wchan != NULL)
                unsleep(p);

	p->p_wchan = ident;
	p->p_wmesg = wmesg;
	/* We don't have these fields. */
#ifndef OSKIT
        p->p_slptime = 0;
        p->p_priority = priority & PRIMASK;
#endif

        TAILQ_INSERT_TAIL(&slpque[LOOKUP(ident)], p, p_procq);
        if (timo)
                thandle = timeout(endtsleep, (void *)p, timo);

	p->p_stat = SSLEEP;
	p->p_stats->p_ru.ru_nvcsw++;

	interrupted = OSKIT_FREEBSD_SLEEP(p);
	if (interrupted)
		unsleep(p);

	splx(s);
#if 0
	p->p_flag &= ~P_SINTR;
#endif
	if (p->p_flag & P_TIMEOUT) {
		p->p_flag &= ~P_TIMEOUT;
		return (EWOULDBLOCK);
	} else if (timo)
		untimeout(endtsleep, (void *)p, thandle);
	if (catch && interrupted)
		return (EINTR);

	return (0);
}

/*
 * Implement timeout for tsleep.
 * If process hasn't been awakened (wchan non-zero),
 * set timeout flag and undo the sleep.  If proc
 * is stopped, just unsleep so it will remain stopped.
 */
void
endtsleep(arg)
	void *arg;
{
	register struct proc *p;
	int s;

	p = (struct proc *)arg;
	s = splhigh();
	if (p->p_wchan) {
		if (p->p_stat == SSLEEP) {
			unsleep(p);
			p->p_stat = SRUN;
			osenv_wakeup(&p->p_sr, OSENV_SLEEP_WAKEUP);
		}
		else
			unsleep(p);
		p->p_flag |= P_TIMEOUT;
	}
	splx(s);
}

#if 0
/*
 * Short-term, non-interruptable sleep.
 */
void
sleep(ident, priority)
	void *ident;
	int priority;
{
	register struct proc *p = curproc;
	register struct slpque *qp;
	register s;

#ifdef DIAGNOSTIC
	if (priority > PZERO) {
		printf("sleep called with priority %d > PZERO, wchan: %p\n",
		    priority, ident);
		panic("old sleep");
	}
#endif
	s = splhigh();
	if (cold || panicstr) {
		/*
		 * After a panic, or during autoconfiguration,
		 * just give interrupts a chance, then just return;
		 * don't run any other procs or panic below,
		 * in case this is the idle process and already asleep.
		 */
		splx(safepri);
		splx(s);
		return;
	}
#ifdef DIAGNOSTIC
	if (ident == NULL || p->p_stat != SRUN || p->p_back)
		panic("sleep");
#endif
	p->p_wchan = ident;
	p->p_wmesg = NULL;
	qp = &slpque[LOOKUP(ident)];
	if (qp->sq_head == 0)
		qp->sq_head = p;
	else
		*qp->sq_tailp = p;
	*(qp->sq_tailp = &p->p_forw) = 0;
	p->p_stat = SSLEEP;
	p->p_stats->p_ru.ru_nvcsw++;
	osenv_sleep(&p->p_sr);
	splx(s);
}
#endif

/*
 * Remove a process from its wait queue
 */
void
unsleep(p)
	register struct proc *p;
{
	int s;

	s = splhigh();
	if (p->p_wchan) {
		TAILQ_REMOVE(&slpque[LOOKUP(p->p_wchan)], p, p_procq);
		p->p_wchan = 0;
	}
	splx(s);
}


/*
 * Make all processes sleeping on the specified identifier runnable.
 */
void
wakeup(ident)
	register void *ident;
{
        register struct slpquehead *qp;
        register struct proc *p;
	int s;

	s = splhigh();
	qp = &slpque[LOOKUP(ident)];
restart:
        for (p = qp->tqh_first; p != NULL; p = p->p_procq.tqe_next) {
                if (p->p_wchan == ident) {
                        TAILQ_REMOVE(qp, p, p_procq);
                        p->p_wchan = 0;
                        if (p->p_stat == SSLEEP) {
                                p->p_stat = SRUN;
				osenv_wakeup(&p->p_sr, OSENV_SLEEP_WAKEUP);
                                goto restart;
                        }
                }
        }
        splx(s);
}



/*
 * Make a process sleeping on the specified identifier runnable.
 * May wake more than one process if a target prcoess is currently
 * swapped out.
 */
void
wakeup_one(ident)
        register void *ident;
{
        register struct slpquehead *qp;
        register struct proc *p;
        int s;

        s = splhigh();
        qp = &slpque[LOOKUP(ident)];

        for (p = qp->tqh_first; p != NULL; p = p->p_procq.tqe_next) {
                if (p->p_wchan == ident) {
                        TAILQ_REMOVE(qp, p, p_procq);
                        p->p_wchan = 0;
                        if (p->p_stat == SSLEEP) {
                                p->p_stat = SRUN;
				osenv_wakeup(&p->p_sr, OSENV_SLEEP_WAKEUP);
                        }
                }
        }
        splx(s);
}


void
sleepinit()
{
        int i;

        for (i = 0; i < TABLESIZE; i++)
                TAILQ_INIT(&slpque[i]);
}

