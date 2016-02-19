/*
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
 * Entrypoints to the FreeBSD device driver set.
 * Each of these functions causes one or more drivers
 * to be minimally initialized and registered (see driver.h).
 * They do _not_ cause the drivers to probe for devices immediately;
 * this way the OS can have full control over device probing.
 *
 * Calling a given driver initialization entrypoint will cause
 * _only_ the appropriate driver code to be linked in,
 * and not the whole fdev_freebsd menagerie;
 * this way, for example, you can create "driver sets"
 * containing only a single driver.
 */
#ifndef _OSKIT_DEV_FREEBSD_H_
#define _OSKIT_DEV_FREEBSD_H_

#include <oskit/dev/error.h>
#include <oskit/dev/osenv.h>

/*
 * Initialize the freebsd drivers with an appropriate osenv structure.
 */
void oskit_freebsd_init_osenv(oskit_osenv_t *osenv);

/*** Functions to initialize and register classes of drivers ***/
/*
 * These functions print a warning message using osenv_log()
 * if they are unable to initialize one or more drivers,
 * but they still continue on to initialize other drivers.
 */

/* Initialize and register all available FreeBSD device drivers */
void oskit_freebsd_init_devs(void);

/* Initialize and register all device drivers of a particular class */
void oskit_freebsd_init_isa(void);

/* 
 * Initialize the support code for the FreeBSD derived components 
 */
void oskit_freebsd_init(void);

/*** Functions to initialize and register individual device drivers ***/
/*
 * These functions all take no parameters
 * and return an error code if they are unable
 * to initialize and register themselves.
 */

/* ISA device drivers */
#define driver(name, major, count, description, vendor, author) \
	oskit_error_t oskit_freebsd_init_##name(void);
#define instance(drivername, port, irq, drq, maddr, msize)
#include <oskit/dev/freebsd_isa.h>
#undef driver
#undef instance


/*** Back-door interfaces ***/
/*
 * The following functions are "back-door" interfaces to the FreeBSD drivers,
 * used to find a FreeBSD device given its FreeBSD-specific device number.
 * Use with caution.
 */
struct oskit_ttystream;

/* Translate a FreeBSD errno value into an OSKIT error code.  */
oskit_error_t oskit_freebsd_xlate_errno(int freebsd_error);

/* Open a device given its FreeBSD device number.  */
oskit_error_t oskit_freebsd_chardev_open(int major, int minor, int flags,
                                    struct oskit_ttystream **out_tty_stream);

#endif /* _OSKIT_DEV_FREEBSD_H_ */
