/*	$NetBSD: psl.h,v 1.11 1998/09/06 04:22:43 mark Exp $	*/

/*
 * Copyright (c) 1995 Mark Brinicombe.
 * All rights reserved.
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
 *	This product includes software developed by Mark Brinicombe
 *	for the NetBSD Project.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * RiscBSD kernel project
 *
 * psl.h
 *
 * spl prototypes.
 * Eventually this will become a set of defines.
 *
 * Created      : 21/07/95
 */

#include <machine/intr.h>

/*
 * These are the different SPL states
 *
 * Each state has an interrupt mask associated with it which
 * indicate which interrupts are allowed.
 */

#define _SPL_0		0
#define _SPL_SOFTCLOCK	1
#define _SPL_SOFTNET	2
#define _SPL_BIO	3
#define _SPL_NET	4
#define _SPL_SOFTSERIAL	5
#define _SPL_TTY	6
#define _SPL_IMP	7
#define _SPL_AUDIO	8
#define _SPL_CLOCK	9
#define _SPL_HIGH	10
#define _SPL_SERIAL	11
#define _SPL_LEVELS	12

#define spl0()		splx(_SPL_0)
/*#define splsoft()	raisespl(_SPL_SOFT)*/
#define splsoftnet()	raisespl(_SPL_SOFTNET)
#define splsoftclock()	lowerspl(_SPL_SOFTCLOCK)
#define splbio()	raisespl(_SPL_BIO)
#define splnet()	raisespl(_SPL_NET)
#define splsoftserial()	raisespl(_SPL_SOFTSERIAL)
#define spltty()	raisespl(_SPL_TTY)
#define spllpt()	spltty()
#define splimp()	raisespl(_SPL_IMP)
#define splaudio()	raisespl(_SPL_AUDIO)
#define splclock()	raisespl(_SPL_CLOCK)
#define splstatclock()	raisespl(_SPL_CLOCK)
#define splserial()	raisespl(_SPL_SERIAL)
#define splhigh()	raisespl(_SPL_HIGH)

#ifdef OSKIT
#include "osenv.h"
volatile int cpl;

/*
 * Add a mask to cpl, and return the old value of cpl.
 */
static __inline int
raisespl(ncpl)
	register int ncpl;
{
	register int ocpl = cpl;

	cpl = ocpl | ncpl;
	if (!ocpl)
		osenv_intr_disable();

	return (ocpl);
}

/*
 * Restore a value to cpl (unmasking interrupts).  If any unmasked
 * interrupts are pending, call Xspllower() to process them.
 */
static __inline void
splx(ncpl)
	register int ncpl;
{

	cpl = ncpl;
	if (!cpl)	
		osenv_intr_enable();
}

/*
 * Same as splx(), but we return the old value of spl, for the
 * benefit of some splsoftclock() callers.
 */
static __inline int
lowerspl(ncpl)
	register int ncpl;
{
	register int ocpl = cpl;

	cpl = ncpl;
	if (!cpl)	
		osenv_intr_enable();

	return (ocpl);
}

/*
 * These two are for state save/restore across calls out of the component.
 */
static __inline int
reset_spl(void)
{
	int	ocpl = cpl;

	cpl = 0;

	return ocpl;
}

static __inline void
restore_spl(int ncpl)
{
	cpl = ncpl;

	if (cpl)
		osenv_intr_disable();
}
#else
#ifdef _KERNEL
#ifndef _LOCORE
int raisespl	__P((int));
int lowerspl	__P((int));
int splx	__P((int));

void setsoftast		__P((void));
void setsoftclock	__P((void));
void setsoftnet		__P((void));
void setsoftserial	__P((void));
void setsoftintr	__P((u_int intrmask));
#endif

extern int current_spl_level;

extern u_int spl_masks[_SPL_LEVELS];
extern u_int spl_smasks[_SPL_LEVELS];
#endif /* _LOCORE */
#endif /* _KERNEL */

/* End of psl.h */
