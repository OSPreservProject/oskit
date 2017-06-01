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

#include <oskit/arm32/shark/base_irq.h>

/*
 * Hardware interrupt nesting counter and software interrupt flag.
 *
 * XXX note that this is an int not a char as on the x86.
 * XXX should be reconciled.
 */
unsigned int base_irq_nest = BASE_IRQ_SOFTINT_CLEARED;

/*
 * High-level IRQ handler function table;
 * all entries initially point to the unexpected interrupt handler.
 */
void (*base_irq_handlers[BASE_IRQ_COUNT])(struct trap_state *ts) = {
	base_irq_default_handler,
	base_irq_default_handler,
	base_irq_default_handler,
	base_irq_default_handler,
	base_irq_default_handler,
	base_irq_default_handler,
	base_irq_default_handler,
	base_irq_default_handler,
	base_irq_default_handler,
	base_irq_default_handler,
	base_irq_default_handler,
	base_irq_default_handler,
	base_irq_default_handler,
	base_irq_default_handler,
	base_irq_default_handler,
	base_irq_default_handler
};

