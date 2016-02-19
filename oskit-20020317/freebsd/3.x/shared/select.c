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
/*
 * Copyright (c) 1982, 1986, 1989, 1993
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
 *	@(#)sys_generic.c	8.5 (Berkeley) 1/21/94
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/stat.h>
#include <sys/signalvar.h>
#include <sys/kernel.h>
#include <sys/malloc.h>

#include <oskit/com/listener.h>
#include <oskit/com/listener_mgr.h>

/*
 * Record a select request.
 */
void
selrecord(selector, sip)
	struct proc *selector;
	struct selinfo *sip;
{
	/*
	 * write whatever is the selector that's associated with 'curproc'
	 * - which basically tells the object on whose behalf we're
	 * selecting in this selinfo
	 */
	sip->si_sel = selector->p_sel;

	/*
	 * XXX - occasionally, in net/bpf.c for instance, a caller will reset
	 * si_pid to zero, which causes selwakeup to return in BSD
	 * To make matters worse, some callers won't even call selwakeup
	 * if si_pid is zero (kern/tty.c).
	 */
	sip->si_pid = 1;
}

/*
 * Do a wakeup when a selectable event occurs.
 */
void
selwakeup(sip)
	register struct selinfo *sip;
{
	/*
	 * go through our list, and notify all listeners
	 */
	struct listener_mgr	*mgr = sip->si_sel;

	if (!mgr)
		return;

	oskit_listener_mgr_notify(mgr);
}

#if 0

	This is the original implementation for reference:

/*              
 * Record a select request.
 */     
void    
selrecord(selector, sip)
        struct proc *selector;
        struct selinfo *sip;
{
        struct proc *p;
        pid_t mypid;

        mypid = selector->p_pid;
        if (sip->si_pid == mypid)
                return;
        if (sip->si_pid && (p = pfind(sip->si_pid)) &&
            p->p_wchan == (caddr_t)&selwait)
                sip->si_flags |= SI_COLL;
        else
                sip->si_pid = mypid;
}

/*
 * Do a wakeup when a selectable event occurs.
 */
void
selwakeup(sip)
        register struct selinfo *sip;
{
        register struct proc *p;
        int s;

        if (sip->si_pid == 0)
                return;
        if (sip->si_flags & SI_COLL) {
                nselcoll++;
                sip->si_flags &= ~SI_COLL;
                wakeup((caddr_t)&selwait);
        }
        p = pfind(sip->si_pid);
        sip->si_pid = 0;
        if (p != NULL) {
                s = splhigh();
                if (p->p_wchan == (caddr_t)&selwait) {
                        if (p->p_stat == SSLEEP)
                                setrunnable(p);
                        else
                                unsleep(p);
                } else if (p->p_flag & P_SELECT)
                        p->p_flag &= ~P_SELECT;
                splx(s);
        }
}

#endif
