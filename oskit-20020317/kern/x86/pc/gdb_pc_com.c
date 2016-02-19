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
#include <oskit/x86/base_idt.h>
#include <oskit/x86/base_gdt.h>
#include <oskit/x86/pc/pic.h>
#include <oskit/x86/pc/base_irq.h>
#include <oskit/x86/pc/com_cons.h>
#include <oskit/config.h>

#ifdef KNIT
void set_irq_handler(int irq, void (*handler)(void));
#endif

extern void gdb_pc_com_intr();

int (*base_trap_gdb_handler)(struct trap_state *ts);

static int gdb_com_port = 1;

int gdb_cons_getchar()
{
        return com_cons_getchar(gdb_com_port);
}

void gdb_cons_putchar(int ch)
{
        com_cons_putchar(gdb_com_port, ch);
}


void gdb_pc_com_init(int com_port, struct termios *com_params)
{
	int com_irq = com_port & 1 ? 4 : 3;

	/* Tell the generic serial GDB code
	   how to send and receive characters.  */
	gdb_serial_recv = gdb_cons_getchar;
	gdb_serial_send = gdb_cons_putchar;
	gdb_com_port = com_port;

	/* Tell the GDB proxy trap handler to use the serial stub.  */
	gdb_signal = gdb_serial_signal;

	/*
	 * Stick in gdb's special trap handler.
	 * This gets called before base_trap_handlers is consulted.
	 * If it returns zero, the normal handler is circumvented entirely;
	 * if it returns nonzero, the normal handler runs next.
	 */
	base_trap_gdb_handler = gdb_trap_ss;

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

#ifdef KNIT
        set_irq_handler(com_irq,gdb_pc_com_intr);
	com_cons_enable_receive_interrupt(com_port);
#else
	/* Hook the COM port's hardware interrupt.
	   The com_cons itself uses only polling for communication;
	   the interrupt is only used to allow the remote debugger
	   to stop us at any point, e.g. when the user presses CTRL-C.  */
	fill_irq_gate(com_irq, (unsigned)gdb_pc_com_intr, KERNEL_CS, ACC_PL_K);

	/* Enable the COM port interrupt.  */
	com_cons_enable_receive_interrupt(com_port);
	pic_enable_irq(com_irq);
#endif
}


