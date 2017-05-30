/*
 * Copyright (c) 1999 University of Utah and the Flux Group.
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
 * Copyright (c) 1994-1997 Mark Brinicombe.
 * Copyright (c) 1994 Brini.
 * All rights reserved.
 *
 * This code is derived from software written for Brini by Mark Brinicombe
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
 *	This product includes software developed by Brini.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BRINI ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL BRINI OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * RiscBSD kernel project
 */

#ifndef _OSKIT_ARM32_BASE_TRAP_H_
#define _OSKIT_ARM32_BASE_TRAP_H_

#ifndef ASSEMBLER
#include <oskit/compiler.h>

/*
 * Trap state derived from NetBSD 1.4
 */
struct trap_state
{
	unsigned int spsr;
	unsigned int r0;
	unsigned int r1;
	unsigned int r2;
	unsigned int r3;
	unsigned int r4;
	unsigned int r5;
	unsigned int r6;
	unsigned int r7;
	unsigned int r8;
	unsigned int r9;
	unsigned int r10;
	unsigned int r11;
	unsigned int r12;
	unsigned int usr_sp;
	unsigned int usr_lr;
	unsigned int trapno;
	unsigned int intno;
	unsigned int svc_sp;
	unsigned int svc_lr;
	unsigned int pc;
};
#endif

#ifdef ASSEMBLER
#define TRAP_STATE_TRAPNO	64
#define TRAP_STATE_INTNO	68
#define TRAP_STATE_PC		80
#endif

#ifndef ASSEMBLER
OSKIT_BEGIN_DECLS
/*
 * This library routine initializes the trap vector in page zero
 * to point to default trap handler entrypoints
 * which merely push the standard trap saved-state frame above
 * and call the default trap handler routine, 'trap_handler'.
 */
extern void base_trap_init(void);


/*
 * There are 8 trap vectors, according to the ARM spec, plus there is
 * an extra software defined trap to handle stack overflow.
 */
#define BASE_TRAP_COUNT		9

/*
 * In the base oskit environment, each trap can have an associated
 * trap handler in the following table. The default trap entrypoint
 * code checks this function pointer (after saving the trap_state
 * frame described by the structure above), and if non-null, calls the
 * C function it points to.  The function can examine and modify the
 * provided trap_state as appropriate.  If the trap handler function
 * returns zero, execution will be resumed or resterted with the final
 * trap_state; if the trap handler function returns nonzero, the trap
 * handler dumps the trap state to the console and panics.
 */
extern int (*base_trap_handlers[])(struct trap_state *ts);

/*
 * Default trap handler for unexpected traps, which simply calls
 * trap_dump_panic.
 */
int base_trap_default_handler(struct trap_state *ts);


/*
 * This routine dumps the contents of the specified trap_state structure
 * to the console using printf.
 * It is normally called automatically from trap_dump_panic
 * when an unexpected trap occurs;
 * however, can be called at other times as well
 * (e.g., for debugging custom trap handlers).
 */
void trap_dump(const struct trap_state *st);


/*
 * This routine simply calls trap_dump to dump the trap state,
 * and then calls panic.
 * It is used by the default trap entrypoint code
 * when a trap occurs and there is no trap handler,
 * and by the UNEXPECTED_TRAP assembly language macro below.
 */
void trap_dump_panic(const struct trap_state *st);


/*
 * External entrypoint for modules that catch particular traps (like svm)
 * and want to turn them into delivered signals.
 */
int oskit_sendsig(int signo, struct trap_state *ts);

/*
 * Generic trap handler for converting any trap into a signal. Can
 * be called by external modules that do not know what to do with a
 * trap. Both entrypoints depend on the application C library having 
 * called base_signal_init() to tell the kernel signal code that it
 * wants to receive signals. Otherwise, traps will continue to panic.
 */
int sendsig_trap_handler(struct trap_state *ts);
OSKIT_END_DECLS
#endif

#endif /* _OSKIT_ARM32_BASE_TRAP_H_ */
