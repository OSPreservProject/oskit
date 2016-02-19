/*
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
 * This file lists the current set of FreeBSD ISA device drivers,
 * as well as the standard locations at which they commonly appear.
 * (These locations indicate where the drivers will probe by default
 * if they are asked to probe with no configuration information available.)
 * This list is used for building libfdev_freebsd itself,
 * by <oskit/dev/freebsd.h> to declare all the function prototypes,
 * and can be used by clients of the OSKIT as well
 * as an automated way to stay up-to-date with the supported drivers.
 *
 * Each driver is listed on a separate line in a standard format,
 * which can be parsed by the C preprocessor or other text tools like awk:
 *
 * driver(drivername, major, count, description, vendor, author)
 *
 * drivername is the abbreviated name of the device driver, e.g., 'sio'.
 * major is the major number assigned to this device driver by FreeBSD.
 * count is the maximum number of device instances that should be supported.
 * description is the longer device name, e.g., 'PC serial port'.
 * vendor is the name of the hardware vendor supplying this device.
 * author is the name of the author(s) of the device driver.
 *
 * Each device instance line has the following format:
 *
 * instance(drivername, port, irq, drq, maddr, msize)
 *
 * drivername is the abbreviated name of the device driver, as above.
 * port is the I/O port at which to probe, or -1 if unknown (wildcard).
 * irq is the interrupt mask to use, or 0 if unknown (wildcard).
 * drq is the PC DMA line to use, or -1 if unknown (wildcard).
 * maddr is the address of the mapped memory region to use, 0 if unknown.
 * msize is the size of the mapped memory region to use, 0 if unknown.
 *
 * The set of instance lines included here corresponds to
 * the set of enabled ISA device entries in FreeBSD's GENERIC configuration.
 */
driver(sc, 12, 1, "PC system console", NULL, "Søren Schmidt")
instance(sc, 0x060, IRQ1, -1, 0, 0)

driver(sio, 28, 4, "PC serial port", NULL, NULL)
instance(sio, 0x3f8, IRQ4, -1, 0, 0)
instance(sio, 0x2f8, IRQ3, -1, 0, 0)
driver(cx, 42, 1, "Cronyx-Sigma adapter", "Cronyx Ltd.", "Serge Vakulenko")
driver(cy, 48, 1, "Cyclades Cyclom-Y serial board", "Cyclades", "Andrew Herbert")
driver(rc, 63, 1, "RISCom/8 serial board", "SDL Communications", "Pavel Antonov, Andrey A. Chernov")
driver(si, 68, 1, "Specialix serial line multiplexor", "Specialix International", "Andy Rutter, Peter Wemm")

/* XXX don't include lpt for now because it does funky block device things */
/* driver(lpt, 16, 1, "Parallel printer port", NULL, "William Jolitz") */
/* instance(lpt, -1, IRQ7, -1, 0, 0) */

driver(mse, 27, 1, "Bus mouse", NULL, "Rick Macklem")
instance(mse, 0x23c, IRQ5, -1, 0, 0)
driver(psm, 21, 1, "PS/2 mouse", NULL, "Sandi Donno")
instance(psm, 0x060, IRQ12, -1, 0, 0)

/* XXX don't include joystick for now because it does funky timer things */
/* driver(joy, 51, 1, "Joystick", NULL, "Jean-Marc Zucconi") */
