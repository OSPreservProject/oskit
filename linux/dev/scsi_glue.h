/*
 * Copyright (c) 1997, 1998 The University of Utah and the Flux Group.
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
 * This file generically defines a small piece of glue code
 * to be built individually for each SCSI device driver,
 * allowing each driver to be independently linkable
 * while still presenting a common fdev interface.
 * Some horrible makefile kludges (see the GNUmakerules file)
 * produce a bunch of <driver>_scsiglue.c files,
 * each of which simply consists of a #include "scsi_glue.h" (this file)
 * followed by the appropriate driver definition line in <dev/linux_scsi.h>.
 */
#ifndef _LINUX_DEV_SCSI_GLUE_H_
#define _LINUX_DEV_SCSI_GLUE_H_

#include <linux/blk.h>
#include <linux/version.h>

#include <asm/types.h>

#include "scsi.h"
#include "hosts.h"

#include "glue.h"

struct scsi_driver_struct {
	struct driver_struct	ds;
	oskit_devinfo_t		dev_info;
	Scsi_Host_Template	tmpl;
	int			initialized;
};

/* Common fdev probe routine for all Linux SCSI drivers */
extern oskit_error_t oskit_linux_scsi_probe(struct driver_struct *ds);

/* This macro is instantiated once for each driver in a separate .o file. */
#define driver(name, description, vendor, author, filename, template)	\
	static struct scsi_driver_struct fdev_drv = {			\
		{ { &oskit_linux_driver_ops },				\
		  oskit_linux_scsi_probe,				\
		  { #name, description " SCSI driver", vendor,		\
		    author, "Linux "UTS_RELEASE } },			\
		{ #name, description " SCSI host adaptor", vendor,	\
		  author, "Linux "UTS_RELEASE },			\
		template };						\
	oskit_error_t oskit_linux_init_scsi_##name(void) {		\
		return oskit_linux_driver_register(&fdev_drv.ds);	\
	}

#endif /* _LINUX_DEV_SCSI_GLUE_H_ */
