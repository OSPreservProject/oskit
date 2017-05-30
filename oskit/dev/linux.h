/*
 * Copyright (c) 1997-2000 University of Utah and the Flux Group.
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
 * Entrypoints to the Linux device driver set.
 * Each of these functions causes one or more drivers
 * to be minimally initialized and registered (see driver.h).
 * They do _not_ cause the drivers to probe for devices immediately;
 * this way the OS can have full control over device probing.
 *
 * Calling a given driver initialization entrypoint will cause
 * _only_ the appropriate driver code to be linked in,
 * and not the whole fdev_linux menagerie;
 * this way, for example, you can create "driver sets"
 * containing only a single driver.
 */
#ifndef _OSKIT_DEV_LINUX_H_
#define _OSKIT_DEV_LINUX_H_

#include <oskit/dev/error.h>
#include <oskit/dev/osenv.h>
#include <oskit/compiler.h>

OSKIT_BEGIN_DECLS

/*
 * Initialize the linux drivers with an appropriate osenv structure.
 */
void oskit_linux_init_osenv(oskit_osenv_t *osenv);

/*** Functions to initialize and register classes of drivers ***/
/*
 * These functions print a warning message using osenv_log()
 * if they are unable to initialize one or more drivers,
 * but they still continue on to initialize other drivers.
 */

/* Initialize and register all available Linux device drivers */
void oskit_linux_init_devs(void);

/* Initialize and register all device drivers of a particular class */
void oskit_linux_init_scsi(void);
void oskit_linux_init_ethernet(void);
void oskit_linux_init_sound(void);

/* Initialize drivers providing a given type of fdev device interface */
void oskit_linux_init_blk(void);
void oskit_linux_init_net(void);


/*** Functions to initialize and register individual device drivers ***/
/*
 * These functions all take no parameters
 * and return an error code if they are unable
 * to initialize and register themselves.
 */

/* Special drivers handled individually */
oskit_error_t oskit_linux_init_ide(void);
oskit_error_t oskit_linux_init_floppy(void);

/* SCSI device drivers */
#define driver(name, description, vendor, author, filename, template) \
	oskit_error_t oskit_linux_init_scsi_##name(void);
#include <oskit/dev/linux_scsi.h>
#undef driver

/* Ethernet device drivers */
#define driver(name, description, vendor, author, filename, probe) \
	oskit_error_t oskit_linux_init_ethernet_##name(void);
#include <oskit/dev/linux_ethernet.h>
#undef driver


/*** Back-door interfaces ***/
/*
 * The following functions are "back-door" interfaces to the Linux drivers,
 * used to find a Linux device given its Linux-specific name or device number.
 * Use with caution.
 */
struct oskit_blkio;
struct gendisk;
struct oskit_netdev;
struct net_io;

/*
 * Linux block-style devices can be opened
 * either by their Linux name or their Linux major/minor number.
 */
oskit_error_t oskit_linux_block_open(const char *name, unsigned flags,
				   struct oskit_blkio **out_io);
oskit_error_t oskit_linux_block_open_kdev(int kdev, struct gendisk *gd,
					unsigned flags,
					struct oskit_blkio **out_io);

/*
 * Network devices live in a separate namespace in Linux,
 * not corresponding to any normal major/minor numbers.
 */
oskit_error_t oskit_linux_netdev_find(const char *name,
				 struct oskit_netdev **out_netdev);
oskit_error_t oskit_linux_net_open(const char *name, unsigned flags,
				 struct net_io *in_io, struct net_io **out_io);

void *oskit_linux_skbmem_alloc(unsigned int size, int flags,
			       unsigned int *realsize);
void oskit_linux_skbmem_free(void *ptr, unsigned int size);

OSKIT_END_DECLS

#endif /* _OSKIT_DEV_LINUX_H_ */
