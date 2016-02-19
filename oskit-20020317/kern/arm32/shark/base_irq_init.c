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
 * IRQ initialization for the oskit's base execution environment.
 */

#include <oskit/arm32/proc_reg.h>
#include <oskit/arm32/base_trap.h>
#include <oskit/arm32/shark/pic.h>
#include <oskit/arm32/pio.h>
#include <oskit/arm32/shark/base_irq.h>
#include <oskit/arm32/base_trap.h>
#include <oskit/arm32/trap.h>
#include <oskit/debug.h>

/*
 * Initialize the system to handle hardware interrupts properly.
 */
void
base_irq_init()
{
	assert(interrupts_enabled() == 0);

	/* Set the trap handler for IRQs */
	base_trap_handlers[T_IRQ] = base_irq_trap_handler;

	/* Program the PIC to our preferred location */
	pic_init(BASE_IRQ_MASTER_BASE, BASE_IRQ_SLAVE_BASE);

	/* Disable all the PIC's IRQ lines */
	pic_disable_all();

	/* Enable processor interrupts so we can take them when we want to */
	enable_interrupts();
}
