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
 * Definitions for entrypoints to "native" fdev device drivers -
 * i.e., device drivers specifically written for the fdev framework,
 * as opposed to being snarfed from some other existing OS.
 * XXX maybe these should just go in mem.h, isa.h, etc.
 */
#ifndef _OSKIT_DEV_NATIVE_H_
#define _OSKIT_DEV_NATIVE_H_


/*** Function entrypoints for the standard processor memory bus driver */

/*
 * Initialize the processor memory bus driver
 * and attach the memory bus device node to the root bus.
 * It is harmless to initialize more than once.
 */
oskit_error_t osenv_membus_init(void);

/*
 * This function returns a reference to the standard memory bus device node,
 * which represents the processor's local physical memory bus.
 * This device node generally represents the root of the hardware tree.
 */
struct oskit_membus *osenv_membus_getbus(void);


/*** Function entrypoints for the standard ISA bus device driver ***/
/* XXX move to oskit/x86/pc/dev.h. */

/*
 * Initialize the ISA bus device driver
 * and attach the ISA device node to the root bus.
 * The device node can later be moved, if appropriate,
 * e.g., to become a child of a PCI bus node
 * representing the PCI bus on which the ISA bridge resides.
 * It is harmless to initialize more than once.
 */
oskit_error_t osenv_isabus_init(void);

/*
 * This function returns a reference to the standard ISA bus device node,
 * which represents the PC's main (and, in general, only) ISA bus.
 * Must only be called after a successful osenv_isabus_init().
 */
struct oskit_isabus *osenv_isabus_getbus(void);

oskit_error_t osenv_isabus_addchild(oskit_u32_t addr, oskit_device_t *dev);
oskit_error_t osenv_isabus_remchild(oskit_u32_t addr);


#endif /* _OSKIT_DEV_NATIVE_H_ */
