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
 * This file generically defines a small piece of glue code
 * to be built individually for each ISA device driver,
 * allowing each driver to be independently linkable
 * while still presenting a common OSKit device interface.
 * Some horrible makefile kludges (see the GNUmakerules file)
 * produce a bunch of <driver>_isaglue.c files,
 * each of which simply consists of a #include "isa_glue.h" (this file)
 * followed by the appropriate driver definition lines in <dev/freebsd_isa.h>.
 */
#ifndef _FREEBSD_DEV_ISA_GLUE_H_
#define _FREEBSD_DEV_ISA_GLUE_H_

#include <oskit/dev/isa.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/tty.h>

#include <i386/isa/isa_device.h>
#include <i386/isa/icu.h>

#include "version.h"

struct glue_isa_driver {
	oskit_isa_driver_t	drvi;
	int			major;
	struct isa_driver	*drv;
	int			count;
	struct glue_isa_device	*devs;
	oskit_devinfo_t		dev_info;
	oskit_devinfo_t		drv_info;
};

extern void oskit_freebsd_init(void);

/* Common routine to initialize the ISA glue module */
extern void oskit_freebsd_isa_init(void);

/* Common routine to register a FreeBSD ISA driver */
extern oskit_error_t oskit_freebsd_isa_register(struct glue_isa_driver *drv,
					      struct cdevsw *cdevsw);


/* Assign unique IDs to all ISA devices, for use in the id_id field. */
#define ISA_DEVICE_MIN 2	/* inclusive */
enum {
	ISA_DEVICE_UNUSED = (ISA_DEVICE_MIN-1),
#define driver(name, major, count, description, vendor, author)		\
	ISA_DEVICE_##name,						\
	ISA_DEVICE_##name##_LAST = ISA_DEVICE_##name + count - 1,
#define instance(name, port, irq, drq, maddr, msize)
#include <oskit/dev/freebsd_isa.h>
#undef driver
#undef instance
	ISA_DEVICE_MAX	/* exclusive */
};


/*
 * This macro is instantiated once for each driver in a separate .o file.
 * The devs field will be filled in at run-time by glue code.
 */
#define driver(name, major, count, description, vendor, author)		\
	extern struct isa_driver name##driver;				\
	extern inthand2_t name##intr;					\
	static struct glue_isa_driver gdrv = {				\
		{NULL}, major, &name##driver, count, NULL,		\
		{ #name, description " driver", vendor,			\
		  author, "FreeBSD "REVISION"-"BRANCH },		\
		{ #name, description, vendor,				\
		  author, "FreeBSD "REVISION"-"BRANCH }			\
	};								\
	oskit_error_t oskit_freebsd_init_##name(void) {			\
		int i = ISA_DEVICE_##name - ISA_DEVICE_MIN;		\
		oskit_freebsd_isa_init();

/*
 * This macro is instantiated for each default instance.
 * The unit and enabled fields will be filled in at run-time by glue code.
 */
#define instance(name, port, irq, drq, maddr, msize)			\
	isa_devtab_tty[i].id_driver = &name##driver;			\
	isa_devtab_tty[i].id_iobase = port;				\
	isa_devtab_tty[i].id_irq = irq;					\
	isa_devtab_tty[i].id_drq = drq;					\
	isa_devtab_tty[i].id_maddr = maddr;				\
	isa_devtab_tty[i].id_msize = msize;				\
	isa_devtab_tty[i].id_intr = name##intr;				\
	isa_devtab_tty[i].id_unit = i -					\
				(ISA_DEVICE_##name - ISA_DEVICE_MIN);	\
	isa_devtab_tty[i].id_enabled = 1;				\
	i++;

/* This macro terminates the device instance array started in driver(). */
#define enddriver(name)							\
		while (i <= ISA_DEVICE_##name##_LAST - ISA_DEVICE_MIN) { \
			isa_devtab_tty[i].id_driver = &name##driver;	\
			isa_devtab_tty[i].id_unit = i -			\
				(ISA_DEVICE_##name - ISA_DEVICE_MIN);	\
			i++;						\
		}							\
		return oskit_freebsd_isa_register(&gdrv, &gcdevsw);	\
	}


/* Lots of bogus defines for shorthand purposes */
#define noopen		(d_open_t *)enodev
#define noclose		(d_close_t *)enodev
#define noread		(d_rdwr_t *)enodev
#define nowrite		noread
#define noioc		(d_ioctl_t *)enodev
#define nostop		(d_stop_t *)enodev
#define noreset		(d_reset_t *)enodev
#define noselect	(d_select_t *)enodev
#define nommap		(d_mmap_t *)enodev
#define nostrat		(d_strategy_t *)enodev
#define nodump		(d_dump_t *)enodev
#define	nodevtotty	(d_ttycv_t *)nullop

#define nxopen		(d_open_t *)enxio
#define	nxclose		(d_close_t *)enxio
#define	nxread		(d_rdwr_t *)enxio
#define	nxwrite		nxread
#define	nxstrategy	(d_strategy_t *)enxio
#define	nxioctl		(d_ioctl_t *)enxio
#define	nxdump		(d_dump_t *)enxio
#define nxstop		(d_stop_t *)enxio
#define nxreset		(d_reset_t *)enxio
#define nxselect	(d_select_t *)enxio
#define nxmmap		nommap		/* must return -1, not ENXIO */
#define	nxdevtotty	(d_ttycv_t *)nullop

#define nullopen	(d_open_t *)nullop
#define nullclose	(d_close_t *)nullop
#define nullstop	(d_stop_t *)nullop
#define nullreset	(d_reset_t *)nullop

#endif /* _FREEBSD_DEV_ISA_GLUE_H_ */
