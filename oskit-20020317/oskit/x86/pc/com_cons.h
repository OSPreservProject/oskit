/*
 * Copyright (c) 1994-1996 Sleepless Software
 * Copyright (c) 1997-1998 University of Utah and the Flux Group.
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
 * Simple polling serial console for the Flux OS toolkit
 */
#ifndef _OSKIT_X86_PC_COM_CONS_H_
#define _OSKIT_X86_PC_COM_CONS_H_

#include <oskit/compiler.h>

struct termios;

OSKIT_BEGIN_DECLS
/*
 * This routine must be called once to initialize the COM port.
 * The com_port parameter specifies which COM port to use, 1 through 4.
 * The supplied termios structure indicates the baud rate and other settings.
 * If com_params is NULL, a default of 9600,8,N,1 is used.
 */
void com_cons_init(int com_port, struct termios *com_params);

/*
 * Primary serial character I/O routines.
 */
int com_cons_getchar(int port);
int com_cons_trygetchar(int port);
void com_cons_putchar(int port, int ch);

/*
 * Since the com_console operates by polling,
 * there is no need to handle serial interrupts in order to do basic I/O.
 * However, if you want to be notified up when a character is received,
 * call this function immediately after com_cons_init(),
 * and make sure the appropriate IDT entry is initialized properly.
 * For example, the serial debugging code for the PC COM port
 * uses this so that the program can be woken up
 * when the user presses CTRL-C from the remote debugger.
 */
void com_cons_enable_receive_interrupt(int port);


/*
 * This waits until the transmit FIFOs are empty on the serial port
 * specified.  This is useful as it allows the programmer to KNOW
 * the the message sent out has been received.  This is necessary
 * before resetting the UART or changing settings if it is desirable
 * for any data already `sent' to actually be transmitted.
 */
void com_cons_flush(int port);
OSKIT_END_DECLS

#endif /* _OSKIT_X86_PC_COM_CONS_H_ */
