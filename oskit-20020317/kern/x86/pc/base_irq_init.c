/*
 * Copyright (c) 1996-1998 University of Utah and the Flux Group.
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

#include <oskit/x86/eflags.h>
#include <oskit/x86/proc_reg.h>
#include <oskit/x86/gate_init.h>
#include <oskit/x86/base_gdt.h>
#include <oskit/x86/base_idt.h>
#include <oskit/x86/base_trap.h>
#include <oskit/x86/pc/pic.h>
#include <oskit/x86/pc/base_irq.h>
#include <oskit/debug.h>

/*
 * Initialize the system to handle hardware interrupts properly.
 */
void
base_irq_init()
{
	assert((get_eflags() & EFL_IF) == 0);

	/* Initialize the IDT vectors used for hardware interrupts */
	gate_init(base_idt, base_irq_inittab, KERNEL_CS);

	/* Program the PIC to our preferred location */
	pic_init(BASE_IRQ_MASTER_BASE, BASE_IRQ_SLAVE_BASE);
	irq_master_base = BASE_IRQ_MASTER_BASE;
	irq_slave_base = BASE_IRQ_SLAVE_BASE;

	/* Disable all the PIC's IRQ lines */
	pic_disable_all();
}

