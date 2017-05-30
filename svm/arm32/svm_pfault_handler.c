/*
 * Copyright (c) 1996, 1998, 1999, 2000 University of Utah and the Flux Group.
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

#include <oskit/debug.h>
#include <oskit/arm32/base_paging.h>
#include <oskit/arm32/base_stack.h>
#include <oskit/arm32/base_trap.h>
#include <oskit/dev/dev.h>
#include <oskit/base_critical.h>
#include <signal.h>

#include "svm_internal.h"

/*
 * Trap handler. All we (currently) care about is page faults. Everything
 * else is passed through.
 */
int
svm_page_fault_handler(struct trap_state *ts)
{
	if (ts->trapno == T_DATA_ABORT) {
		int	rcode, enabled;

		enabled = osenv_intr_save_disable();

		/*
		 * XXX: The PC is defined to be at +8 when the trap
		 * vector is invoked (low level trap vector). Should
		 * this adjustment be made earlier?
		 */
		ts->pc -= 8;
		rcode = svm_fault(get_far(), get_fsr());

		if (enabled)
			osenv_intr_enable();

		if (rcode != 0)
			rcode = oskit_sendsig(SIGSEGV, ts);
		
		return rcode;
	}

	/*
	 * Not a page fault. Pass it through to the application as
	 * a signal. If signal handling is not enabled, a trap dump
	 * will be generated.
	 */
	return sendsig_trap_handler(ts);
}

