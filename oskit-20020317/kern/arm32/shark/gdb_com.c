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
 * Remote GDB debugging through COM port on a PC.
 */

#include <oskit/gdb.h>
#include <oskit/gdb_serial.h>
#include <oskit/tty.h>
#include <oskit/arm32/pio.h>
#include <oskit/arm32/base_trap.h>
#include <oskit/arm32/trap.h>
#include <oskit/arm32/shark/pic.h>
#include <oskit/arm32/shark/base_irq.h>
#include <oskit/arm32/shark/com_cons.h>
#include <oskit/config.h>
#include <oskit/debug.h>

#define NULL 0

extern void gdb_pc_com_intr();

static int gdb_com_port = 1;

int gdb_cons_getchar()
{
        return com_cons_getchar(gdb_com_port);
}

void gdb_cons_putchar(int ch)
{
        com_cons_putchar(gdb_com_port, ch);
}

void gdb_trap_bounce(struct trap_state *ss)
{
	/*
	 * Change the trap number number such that gdb_trap knows it
	 * is a hardware interrupt.
	 */
	ss->trapno = -1;
	
	if (gdb_trap(ss)) {
		trap_dump(ss);
		panic("gdb_trap_ss");
	}
}

void gdb_com_init(int com_port, struct termios *com_params)
{
	int com_irq;

	if (com_port == 1)
		com_irq = 4;
	else
		assert(0);

	/* Tell the generic serial GDB code
	   how to send and receive characters.  */
	gdb_serial_recv = gdb_cons_getchar;
	gdb_serial_send = gdb_cons_putchar;
	gdb_com_port = com_port;

	/* Tell the GDB proxy trap handler to use the serial stub.  */
	gdb_signal = gdb_serial_signal;

	/* Hook in the GDB proxy trap handler as the main trap handler
	   for all traps that make sense ... */
	base_trap_handlers[T_UNDEF]          = gdb_trap;	
	base_trap_handlers[T_SWI]            = gdb_trap;	
	base_trap_handlers[T_PREFETCH_ABORT] = gdb_trap;	
	base_trap_handlers[T_DATA_ABORT]     = gdb_trap;	
	base_trap_handlers[T_ADDREXC]        = gdb_trap;	
	
	/*
	 * Initialize the serial port.
	 * If no com_params were specified by the caller,
	 * then default to base_raw_termios.
	 * (If we just passed through the NULL,
	 * then com_cons_init would default to base_cooked_termios,
	 * which messes up the GDB remote debugging protocol.)
	 */
	if (com_params == 0)
		com_params = &base_raw_termios;
	com_cons_init(com_port, com_params);

	/*
	 * IRQs are initialized, so take over the IRQ for the com port.
	 */
	base_irq_handlers[com_irq] = gdb_trap_bounce;

	/* Enable the COM port interrupt.  */
	com_cons_enable_receive_interrupt(com_port);
	pic_enable_irq(com_irq);
}

void
gdb_set_bkpt(void *bkpt)
{
	/*
	 * Set to take a breakpoint at the entrypoint to main().
	 */

	/*
	 * But for now use the same instruction that GDB uses, but with
	 * 1 bit different. Note that gdb_trap will look for this and
	 * increment the PC past this instruction, since it will come in on
	 * an undefined instruction trap.
	 */
	asm volatile(".word 0xE7FFDEFD" : : : "memory");	
}
