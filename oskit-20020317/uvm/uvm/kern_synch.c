/*
 * Copyright (c) 1997-1999, 2001 The University of Utah and the Flux Group.
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

#include "oskit_uvm_internal.h"

int	lbolt;			/* once a second sleep address */

/*
 * We're only looking at 7 bits of the address; everything is
 * aligned to 4, lots of things are aligned to greater powers
 * of 2.  Shift right by 8, i.e. drop the bottom 256 worth.
 */
#define TABLESIZE	128
#undef  LOOKUP
#define LOOKUP(x)	(((int)(x) >> 8) & (TABLESIZE - 1))

/*
 * The global scheduler state.
 */
struct prochd sched_qs[RUNQUE_NQS];	/* run queues */
__volatile u_int32_t sched_whichqs;	/* bitmap of non-empty queues */
struct slpque sched_slpque[TABLESIZE];

struct simplelock sched_lock = SIMPLELOCK_INITIALIZER;

static pthread_cond_t uvm_cond = PTHREAD_COND_INITIALIZER;
static int oskit_uvm_sleep(int *sync)
{
	int s;

	XPRINTF(OSKIT_DEBUG_SYNC, __FUNCTION__": thread %d sleep on %p\n",
		(int)pthread_self(), sync);
	curproc->p_pid = 0;
	s = reset_spl();
	*sync = 0;
	while (*sync == 0) {
		pthread_cond_wait(&uvm_cond, &oskit_uvm_mutex);
	}
	restore_spl(s);
	curproc->p_pid = (int)pthread_self();
	return 0;
}

static void oskit_uvm_wakeup(int *sync)
{
    	XPRINTF(OSKIT_DEBUG_SYNC, __FUNCTION__": wakeup %p\n", sync);
	*sync = 1;
	pthread_cond_broadcast(&uvm_cond);
}

/*
 * General sleep call. This implementation ignores the timeout value,
 * and is a trivialized version of the original tsleep code. However, it
 * appears that the netbsd code requires little else.
 *
 * Suspends the current process until a wakeup is
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
ltsleep(ident, priority, wmesg, timo, interlock)
	void *ident;
	int priority, timo;
	const char *wmesg;
	__volatile struct simplelock *interlock;
{
#ifdef OSKIT
    	struct oskit_uvm_per_thread *pt
	    = pthread_getspecific(oskit_uvm_per_thread_key);
	struct lwp *p = &pt->pt_lwp;
#else
	register struct proc *p = curproc;
#endif
	register struct slpque *qp;
	register int s;
	int interrupted, catch = priority & PCATCH;
	int relock = (priority & PNORELOCK) == 0;

	s = splhigh();

#ifdef OSKIT
	assert(p->p_wchan == 0);
#endif

	p->p_wchan = ident;
	p->p_wmesg = wmesg;

	qp = SLPQUE(ident);
	if (qp->sq_head == 0)
		qp->sq_head = p;
	else
		*qp->sq_tailp = p;
	*(qp->sq_tailp = &p->p_forw) = 0;
	if (interlock != NULL)
		simple_unlock(interlock);

	p->p_stat = SSLEEP;
	interrupted = oskit_uvm_sleep(&p->p_sync);
	if (interrupted)
		unsleep(p);
	splx(s);
	if (catch && interrupted) {
	    if (relock && interlock != NULL)
		simple_lock(interlock);
	    return (EINTR);
	}
	if (relock && interlock != NULL)
		simple_lock(interlock);
	return (0);
}

void
sleep(void *chan, int pri)
{
	tsleep(chan, pri, "sleep", 0);
}

/*
 * Remove a process from its wait queue
 */
void
unsleep(p)
#ifdef OSKIT
	register struct lwp *p;
#else
	register struct proc *p;
#endif
{
	register struct slpque *qp;
#ifdef OSKIT
	register struct lwp **hp;
#else
	register struct proc **hp;
#endif
	int s;

	s = splhigh();
	if (p->p_wchan) {
		hp = &(qp = SLPQUE(p->p_wchan))->sq_head;
		while (*hp != p)
			hp = &(*hp)->p_forw;
		*hp = p->p_forw;
		if (qp->sq_tailp == &p->p_forw)
			qp->sq_tailp = hp;
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
	register struct slpque *qp;
#ifdef OSKIT
	register struct lwp *p, **q;
#else
	register struct proc *p, **q;
#endif
	int s;

	s = splhigh();
	qp = SLPQUE(ident);
restart:
	for (q = &qp->sq_head; *q; ) {
		p = *q;
		if (p->p_wchan == ident) {
			p->p_wchan = 0;
			*q = p->p_forw;
			if (qp->sq_tailp == &p->p_forw)
				qp->sq_tailp = q;
			if (p->p_stat == SSLEEP) {
				p->p_stat = SRUN;
				oskit_uvm_wakeup(&p->p_sync);
				goto restart;
			}
		} else
			q = &p->p_forw;
	}
	splx(s);
}


/*
 * Optimized-for-wakeup() version of setrunnable().
 */
__inline void
#ifdef OSKIT
awaken(struct lwp *p)
#else
awaken(struct proc *p)
#endif
{

	SCHED_ASSERT_LOCKED();

#ifndef OSKIT
	if (p->p_slptime > 1)
		updatepri(p);
	p->p_slptime = 0;
#endif
	p->p_stat = SRUN;

#ifdef OSKIT
	oskit_uvm_wakeup(&p->p_sync);
#else
	/*
	 * Since curpriority is a user priority, p->p_priority
	 * is always better than curpriority.
	 */
	if (p->p_flag & P_INMEM) {
		setrunqueue(p);
		KASSERT(p->p_cpu != NULL);
		need_resched(p->p_cpu);
	} else
		sched_wakeup(&proc0);
#endif
}


/*
 * Make the highest priority process first in line on the specified
 * identifier runnable.
 */
void
wakeup_one(void *ident)
{
	struct slpque *qp;
#ifdef OSKIT
	struct lwp *p, **q;
	struct lwp *best_sleepp, **best_sleepq;
	struct lwp *best_stopp, **best_stopq;
#else
	struct proc *p, **q;
	struct proc *best_sleepp, **best_sleepq;
	struct proc *best_stopp, **best_stopq;
#endif
	int s;

	best_sleepp = best_stopp = NULL;
	best_sleepq = best_stopq = NULL;

	SCHED_LOCK(s);

	qp = SLPQUE(ident);

	for (q = &qp->sq_head; (p = *q) != NULL; q = &p->p_forw) {
#ifdef DIAGNOSTIC
		if (p->p_back || (p->p_stat != SSLEEP && p->p_stat != SSTOP))
			panic("wakeup_one");
#endif
		if (p->p_wchan == ident) {
			if (p->p_stat == SSLEEP) {
				if (best_sleepp == NULL ||
				    p->p_priority < best_sleepp->p_priority) {
					best_sleepp = p;
					best_sleepq = q;
				}
			} else {
				if (best_stopp == NULL ||
				    p->p_priority < best_stopp->p_priority) {
					best_stopp = p;
					best_stopq = q;
				}
			}
		}
	}

	/*
	 * Consider any SSLEEP process higher than the highest priority SSTOP
	 * process.
	 */
	if (best_sleepp != NULL) {
		p = best_sleepp;
		q = best_sleepq;
	} else {
		p = best_stopp;
		q = best_stopq;
	}

	if (p != NULL) {
		p->p_wchan = NULL;
		*q = p->p_forw;
		if (qp->sq_tailp == &p->p_forw)
			qp->sq_tailp = q;
		if (p->p_stat == SSLEEP)
			awaken(p);
	}
	SCHED_UNLOCK(s);
}
