/*
 * Copyright (c) 1999, 2000 University of Utah and the Flux Group.
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

#include <oskit/x86/base_trap.h>
#include <oskit/x86/base_cpu.h>

#include "native.h"
#include "support.h"

/*
 * defined by libkern
 */

int phys_mem_max;
int phys_mem_va;
int base_stack_start;
int (*base_trap_handlers[BASE_TRAP_COUNT])(struct trap_state *ts) = { };
struct cpu_info base_cpuid;

/*
 * The C library will call this function to say where we should deliver
 * signals.  The C library expects us to deliver all signals to that
 * address.  Some signals we cannot deliver in a straightforward way,
 * so include only a few that are essential for now.
 */
typedef void (*deliver_func_t)(int, int, struct sigcontext *);

/*
 * See irq.c. This list defines signals that are really traps, not
 * interrupts.
 */
sigset_t	oskitunix_sigtraps;

void oskit_sendsig_init(deliver_func_t f)
{
	/*
	 * These are more like traps than interrupts, but they come in
	 * through the signal path.
	 */
	oskitunix_set_signal_handler(SIGSEGV, f);
	NATIVEOS(sigaddset)(&oskitunix_sigtraps, SIGSEGV);
	oskitunix_set_signal_handler(SIGBUS, f);
	NATIVEOS(sigaddset)(&oskitunix_sigtraps, SIGBUS);
	oskitunix_set_signal_handler(SIGFPE, f);
	NATIVEOS(sigaddset)(&oskitunix_sigtraps, SIGFPE);
	oskitunix_set_signal_handler(SIGILL, f);
	NATIVEOS(sigaddset)(&oskitunix_sigtraps, SIGILL);
}
