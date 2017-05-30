/*
 * Copyright (c) 1996-1998, 2000 University of Utah and the Flux Group.
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

#include <oskit/x86/pc/base_irq.h>

/*
 * The initial values of these variables
 * are the standard locations of the PICs on startup of a PC.
 * We will typically reprogram the PICs (see base_irq_init)
 * so that the master PIC doesn't conflict with processor exception vectors;
 * at that point these variables will be changed to reflect the new locations.
 */
int irq_master_base = 0x08;
int irq_slave_base = 0x70;

/*
 * Hardware interrupt nesting counter and software interrupt flag.
 */
unsigned char base_irq_nest = BASE_IRQ_SOFTINT_CLEARED;

/*
 * High-level IRQ handler function table;
 * all entries initially point to the unexpected interrupt handler.
 */
unsigned int (*base_irq_handlers[BASE_IRQ_COUNT])(struct trap_state *ts) = {
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

