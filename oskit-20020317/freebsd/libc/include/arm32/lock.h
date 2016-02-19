/*
 * Copyright (c) 1997, by Steve Passe
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the developer may NOT be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	$Id: lock.h,v 1.6 1998/04/06 11:38:17 phk Exp $
 */


#ifndef _MACHINE_LOCK_H_
#define _MACHINE_LOCK_H_

#ifdef SMP
#error "Sorry, you'll have to define the arm smp locking functions yourself."
#endif


#define MPINTR_LOCK()
#define MPINTR_UNLOCK()

#define CPL_LOCK()
#define CPL_UNLOCK()
#define SCPL_LOCK()
#define SCPL_UNLOCK()

#define COM_LOCK()
#define COM_UNLOCK()
#define CLOCK_LOCK()
#define CLOCK_UNLOCK()

/*
 * Simple spin lock.
 * It is an error to hold one of these locks while a process is sleeping.
 */
struct simplelock {
	volatile int	lock_data;
};

/* functions in simplelock.s */
void	s_lock_init		__P((struct simplelock *));
void	s_lock			__P((struct simplelock *));
int	s_lock_try		__P((struct simplelock *));
void	s_unlock		__P((struct simplelock *));
void	ss_lock			__P((struct simplelock *));
void	ss_unlock		__P((struct simplelock *));
void	s_lock_np		__P((struct simplelock *));
void	s_unlock_np		__P((struct simplelock *));

/* global data in mp_machdep.c */
extern struct simplelock	imen_lock;
extern struct simplelock	cpl_lock;
extern struct simplelock	fast_intr_lock;
extern struct simplelock	intr_lock;
extern struct simplelock	clock_lock;
extern struct simplelock	com_lock;
extern struct simplelock	mpintr_lock;
extern struct simplelock	mcount_lock;

#if !defined(SIMPLELOCK_DEBUG) && NCPUS > 1
/*
 * This set of defines turns on the real functions in i386/isa/apic_ipl.s.
 */
#define	simple_lock_init(alp)	s_lock_init(alp)
#define	simple_lock(alp)	s_lock(alp)
#define	simple_lock_try(alp)	s_lock_try(alp)
#define	simple_unlock(alp)	s_unlock(alp)

#endif /* !SIMPLELOCK_DEBUG && NCPUS > 1 */


#endif /* !_MACHINE_LOCK_H_ */
