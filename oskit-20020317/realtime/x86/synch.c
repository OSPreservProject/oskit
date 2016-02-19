/*
 * Copyright (c) 1996-2000 University of Utah and the Flux Group.
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

#include <oskit/x86/proc_reg.h>
#include <oskit/x86/eflags.h>
#include <oskit/x86/pc/pic.h>

/* Pending IRQs that could not be delivered or have been soft scheduled. */
extern volatile unsigned int	realtime_irq_pending_irqs;

/* Deliver all pending IRQs to standard handlers */
extern void			realtime_deliver_pending_irqs(void);

/* Software flag to control when the rest of the oskit enables/disables */
extern int			realtime_irq_intr_enabled;

/*
 * Interrupt enable and disable.
 */
inline void
osenv_intr_enable(void)
{
	cli();

	/*
	 * XXX What about a realtime thread calling osenv_intr_enable()?
	 */
	if (realtime_irq_pending_irqs)
		realtime_deliver_pending_irqs();

	realtime_irq_intr_enabled = 1;
	sti();
}

inline void
osenv_intr_disable(void)
{
	realtime_irq_intr_enabled = 0;
}

inline int
osenv_intr_enabled(void)
{
	return realtime_irq_intr_enabled;
}

/*
 * Disable interrupts returning the old value.  Combo of:
 *	save = osenv_intr_enabled();
 *	osenv_intr_disable();
 */
inline int
osenv_intr_save_disable(void)
{
	int ret = realtime_irq_intr_enabled;

	realtime_irq_intr_enabled = 0;
	return ret;
}
