/*
 * Copyright (c) 1994-1996 Sleepless Software
 * Copyright (c) 1997-1999 University of Utah and the Flux Group.
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
 * Remote PC serial-line debugging for the Flux OS Toolkit
 */

#include <oskit/tty.h>
#include <oskit/x86/pc/com_cons.h>

#ifdef KNIT
void set_irq_handler(int irq, void (*handler)(void));
#else
#include <oskit/x86/base_idt.h>
#include <oskit/x86/base_gdt.h>
#include <oskit/x86/pc/pic.h>
#include <oskit/x86/pc/base_irq.h>
#endif

/*
 * Setup a port as a kill switch to force the kernel into a
 * panic. Very useful when the kernel is looping someplace and
 * interrupts are still enabled. Simply cat some characters into the
 * special device file.
 */
void
gdb_pc_set_killswitch(int port)
{
	int com_irq = port & 1 ? 4 : 3;
	extern void com_kill_intr();

	/*
	 * Initialize the serial port with base_raw_termios.
	 */
	com_cons_init(port, &base_raw_termios);

	/*
	 * Drain it.
	 */
	while (com_cons_trygetchar(port) != -1)
		;

	/* Hook the COM port's hardware interrupt. */
#ifdef KNIT
        set_irq_handler(com_irq, com_kill_intr);
	com_cons_enable_receive_interrupt(port);
#else
	fill_irq_gate(com_irq,
		      (unsigned)com_kill_intr, KERNEL_CS, ACC_PL_K);

	/* Enable the COM port interrupt.  */
	com_cons_enable_receive_interrupt(port);
	pic_enable_irq(com_irq);
#endif
}
