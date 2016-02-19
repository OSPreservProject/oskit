/*
 * Copyright (c) 1997, 1998, 1999 The University of Utah and the Flux Group.
 * 
 * This file is part of the OSKit Linux Glue Libraries, which are free
 * software, also known as "open source;" you can redistribute them and/or
 * modify them under the terms of the GNU General Public License (GPL),
 * version 2, as published by the Free Software Foundation (FSF).
 * 
 * The OSKit is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GPL for more details.  You should have
 * received a copy of the GPL along with the OSKit; see the file COPYING.  If
 * not, write to the FSF, 59 Temple Place #330, Boston, MA 02111-1307, USA.
 */
/*
 * Definitions private to the Linux device driver glue code,
 * but global for all classes of Linux driver glue (e.g., block, net, scsi).
 */
#ifndef _LINUX_DEV_GLUE_H_
#define _LINUX_DEV_GLUE_H_

#include <oskit/dev/dev.h>
#include <oskit/dev/driver.h>
#include <oskit/dev/isa.h>
#include "osenv.h"

/*
 * One of these structures is statically created for each device driver.
 * The drvi.ops operations vector must point to &oskit_linux_driver_ops.
 * This way, each Linux driver is represented by a separate fdev driver object,
 * but they all refer to the same implementation functions (in driver.c).
 */
struct driver_struct {
	oskit_isa_driver_t drvi;		/* COM fdev driver interface */
	oskit_error_t (*probe)(struct driver_struct *ds);
					/* Routine to probe for devices */
	oskit_devinfo_t info;		/* Descriptive info on driver */
};

extern struct oskit_isa_driver_ops oskit_linux_driver_ops;


/*
 * Initialize the common data structures used by all Linux drivers.
 * Each device- or class-specific initialization function
 * should call this function before doing anything else.
 * It is harmless to call this function multiple times.
 */
oskit_error_t oskit_linux_dev_init(void);

/*
 * Register a Linux device driver node statically created
 * using the above driver_struct.
 */
oskit_error_t oskit_linux_driver_register(struct driver_struct *drv);


/*
 * Macro to call a single-driver initialization function
 * and log an appropriate message if it fails.
 */
#define INIT_DRIVER(func, name)						\
	({	oskit_error_t rc = oskit_linux_init_##func();		\
		if (rc) osenv_log(OSENV_LOG_ERR,			\
				 "error initializing "name": %d", rc);	\
	})

#endif /* _LINUX_DEV_GLUE_H_ */
