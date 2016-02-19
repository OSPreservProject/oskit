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

#include <termios.h>
#include <assert.h>
#include <oskit/tty.h>
#include <oskit/base_critical.h>
#include <oskit/x86/pio.h>
#include <oskit/x86/pc/com_cons.h>
#include <oskit/c/string.h>

static int ser_io_base[4];
static struct termios ser_termios[4];

int com_cons_trygetchar(int port)
{
	int ch = -1;

	assert(port >= 1 && port <= 4);
	assert(ser_io_base[port] != 0);

	base_critical_enter();
	if (inb(ser_io_base[port] + 5) & 0x01)
		ch = inb(ser_io_base[port] + 0);
	base_critical_leave();
	return ch;
}

int com_cons_getchar(int port)
{
	int ch;

	assert(port >= 1 && port <= 4);
	assert(ser_io_base[port] != 0);

	/* Wait for a character to arrive.  */
	for (;;) {
		base_critical_enter();
		if (inb(ser_io_base[port] + 5) & 0x01) {
			/* Grab it.  */
			ch = inb(ser_io_base[port] + 0);
			break;
		}
		base_critical_leave();
	}
#ifdef	COMCONS_LINEDISC
	if (ser_termios[port].c_lflag & ECHO) {
		/* Wait for the transmit buffer to become available.  */
		while (!(inb(ser_io_base[port] + 5) & 0x20));

		outb(ser_io_base[port] + 0, ch);

		if (ch == '\r') {
			while (!(inb(ser_io_base[port] + 5) & 0x20));

			outb(ser_io_base[port] + 0, '\n');
		}
	}
#endif
	base_critical_leave();

#ifdef	COMCONS_LINEDISC
	if (ser_termios[port].c_iflag & ICRNL)
		if (ch == '\r')
			ch = '\n';
#endif
	return ch;
}

void com_cons_putchar(int port, int ch)
{
	assert(port >= 1 && port <= 4);
	assert(ser_io_base[port] != 0);

	base_critical_enter();

	if (ser_termios[port].c_oflag & OPOST)
		if (ch == '\n')
			com_cons_putchar(port,'\r');

	/* Wait for the transmit buffer to become available.  */
	while (!(inb(ser_io_base[port] + 5) & 0x20));

	outb(ser_io_base[port] + 0, ch);

	base_critical_leave();
}

void com_cons_init(int port, struct termios *com_params)
{
	unsigned char dfr;
	unsigned divisor;

	assert(port >= 1 && port <= 4);

	base_critical_enter();

	switch (port)
	{
		case 1:	ser_io_base[1] = 0x3f8; break;
		case 2:	ser_io_base[2] = 0x2f8; break;
		case 3:	ser_io_base[3] = 0x3e8; break;
		case 4:	ser_io_base[4] = 0x2e8; break;
		default: assert(0);
	}

	/* If no com_params was supplied, use a common default.  */
	if (com_params == 0)
		com_params = &base_cooked_termios;

	/* Determine what to plug in the data format register.  */
	if (com_params->c_cflag & PARENB)
		if (com_params->c_cflag & PARODD)
			dfr = 0x08;
		else
			dfr = 0x18;
	else
		dfr = 0x00;
	if (com_params->c_cflag & CSTOPB)
		dfr |= 0x04;
	switch (com_params->c_cflag & 0x00000300)
	{
		case CS5: dfr |= 0x00; break;
		case CS6: dfr |= 0x01; break;
		case CS7: dfr |= 0x02; break;
		case CS8: dfr |= 0x03; break;
	}

	/* Convert the baud rate into a divisor latch value.  */
	divisor = 115200 / com_params->c_ospeed;

	/* Initialize the serial port.  */
	outb(ser_io_base[port] + 3, 0x80 | dfr);	/* DLAB = 1 */
	outb(ser_io_base[port] + 0, divisor & 0xff);
	outb(ser_io_base[port] + 1, divisor >> 8);
	outb(ser_io_base[port] + 3, 0x03 | dfr);	/* DLAB = 0, frame = 8N1 */
	outb(ser_io_base[port] + 1, 0x00);	/* no interrupts enabled */
	outb(ser_io_base[port] + 4, 0x0b);	/* OUT2, RTS, and DTR enabled */

	/* make sure the FIFO is on */
	outb(ser_io_base[port] + 2, 0x41); /* 4 byte trigger (0x40); on (0x01) */

	/* Clear all serial interrupts.  */
	inb(ser_io_base[port] + 6);	/* ID 0: read RS-232 status register */
	inb(ser_io_base[port] + 2);	/* ID 1: read interrupt identification reg */
	inb(ser_io_base[port] + 0);	/* ID 2: read receive buffer register */
	inb(ser_io_base[port] + 5);	/* ID 3: read serialization status reg */
	memcpy(&ser_termios[port], com_params, sizeof(struct termios));
	base_critical_leave();
}

void com_cons_enable_receive_interrupt(int port)
{
	assert(port >= 1 && port <= 4);
	assert(ser_io_base[port] != 0);

	outb(ser_io_base[port] + 1, 0x01);	/* interrupt on received data */
}


/*
 * This delays until the transmit queue has emptied on the serial port.
 * XXX: It could potentially delay forever. . .
 */
void com_cons_flush(int port)
{
	assert(port >= 1 && port <= 4);
	assert(ser_io_base[port] != 0);

	/*
	 * Serialization Status Register:
	 * bit 6 is 1 if no byte in hold register or shift register.
	 */
	while (!(inb(ser_io_base[port] + 5) & 0x40))
		;

}
