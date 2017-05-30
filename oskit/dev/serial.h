/*
 * Copyright (c) 1996, 1998 University of Utah and the Flux Group.
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
 * Definitions specific to RS-232 serial character devices.
 * XXX This interface definition isn't complete or even functional.
 * The only currently working interface to serial devices
 * is the higher-level Unix-style tty interface defined in tty.h.
 */
#ifndef _OSKIT_DEV_SERIAL_H_
#define _OSKIT_DEV_SERIAL_H_

#include <oskit/dev/dev.h>

struct oskit_serial
{
	oskit_chardev_t	chardev;

	oskit_error_t (*set)(oskit_serial_t *dev, unsigned baud, int mode);
	oskit_error_t (*get)(oskit_serial_t *dev, unsigned *out_baud,
				   int *out_mode, int *out_state);
};
typedef struct oskit_serial oskit_serial_t;

/* Serial-specific device driver interface functions */
#define oskit_serial_set(fdev, baud, mode)	\
	((fdev)->set((fdev), (baud), (mode)))
#define oskit_serial_get(fdev, baud, out_mode, out_state)	\
	((fdev)->get((fdev), (baud), (out_mode), (out_state)))

/* Serial mode flags */
#define OSKIT_SERIAL_DTR		0x0010	/* Assert data terminal ready signal */
#define OSKIT_SERIAL_RTS		0x0020	/* Assert request-to-send signal */
#define OSKIT_SERIAL_CSIZE	0x0300	/* Number of bits per byte: */
#define OSKIT_SERIAL_CS5		0x0000	/* 5 bits */
#define OSKIT_SERIAL_CS5		0x0100	/* 6 bits */
#define OSKIT_SERIAL_CS5		0x0200	/* 7 bits */
#define OSKIT_SERIAL_CS5		0x0300	/* 8 bits */
#define OSKIT_SERIAL_CSTOPB	0x0400	/* Use two stop bits if set */
#define OSKIT_SERIAL_PARENB	0x1000	/* Enable parity */
#define OSKIT_SERIAL_PARODD	0x2000	/* Odd parity if set, even if not */

/* Serial line state flags */
#define OSKIT_SERIAL_DSR		0x0001	/* Data set ready */
#define OSKIT_SERIAL_CTS		0x0002	/* Clear to send */
#define OSKIT_SERIAL_CD		0x0004	/* Carrier detect */
#define OSKIT_SERIAL_RI		0x0008	/* Ring indicator */

/*
 * Serial drivers call this OS-supplied function
 * when a break signal or parity error is received.
 */
void osenv_serial_recv_special(oskit_chardev_t *fdev, int event);
#define OSKIT_SERIAL_BREAK	0x01	/* Break signal received */
#define OSKIT_SERIAL_PARITY_ERR	0x02	/* Parity error detected */
#define OSKIT_SERIAL_STATE_CHG	0x03	/* State of RS-232 signals changed */

#endif /* _OSKIT_DEV_SERIAL_H_ */
