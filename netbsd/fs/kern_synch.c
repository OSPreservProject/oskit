/*
 * Copyright (c) 1997-1999 University of Utah and the Flux Group.
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
#include <sys/proc.h>
#include <sys/kernel.h>
#include "fs_glue.h"
#include <oskit/dev/dev.h>

/*
 * We're only looking at 7 bits of the address; everything is
 * aligned to 4, lots of things are aligned to greater powers
 * of 2.  Shift right by 8, i.e. drop the bottom 256 worth.
 */
#define TABLESIZE	128
#undef  LOOKUP
#define LOOKUP(x)	(((int)(x) >> 8) & (TABLESIZE - 1))
struct slpque {
	struct proc *sq_head;
	struct proc **sq_tailp;
} slpque[TABLESIZE];

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
tsleep(ident, priority, wmesg, timo)
	void *ident;
	int priority, timo;
	char *wmesg;
{
	register struct proc *p = curproc;
	register struct slpque *qp;
	register int s;
	int interrupted, catch = priority & PCATCH;

	s = splhigh();
	p->p_wchan = ident;
	p->p_wmesg = wmesg;
	qp = &slpque[LOOKUP(ident)];
	if (qp->sq_head == 0)
		qp->sq_head = p;
	else
		*qp->sq_tailp = p;
	*(qp->sq_tailp = &p->p_forw) = 0;
	p->p_stat = SSLEEP;
	interrupted = OSKIT_NETBSD_SLEEP(p);
	if (interrupted)
		unsleep(p);
	splx(s);
	if (catch && interrupted)
		return (EINTR);
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
	register struct proc *p;
{
	register struct slpque *qp;
	register struct proc **hp;
	int s;

	s = splhigh();
	if (p->p_wchan) {
		hp = &(qp = &slpque[LOOKUP(p->p_wchan)])->sq_head;
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
	register struct proc *p, **q;
	int s;

	s = splhigh();
	qp = &slpque[LOOKUP(ident)];
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
				osenv_wakeup(&p->p_sr, OSENV_SLEEP_WAKEUP);
				goto restart;
			}
		} else
			q = &p->p_forw;
	}
	splx(s);
}

