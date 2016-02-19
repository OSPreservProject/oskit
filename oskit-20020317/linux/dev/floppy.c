/*
 * Copyright (c) 1997-2000 The University of Utah and the Flux Group.
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
 * This source file overrides the Linux block/floppy.c,
 * but #include's the original floppy.c as the first thing -
 * in other words, we basically extend Linux's floppy.c at the bottom.
 */
#include "../drivers/block/floppy.c"

#include <linux/version.h>

#include <oskit/dev/blk.h>
#include <oskit/dev/linux.h>
#include <oskit/dev/bus.h>
#include <oskit/dev/isa.h>
#include <oskit/dev/native.h>

#include "glue.h"
#include "block.h"
#include "linux_emul.h"
#include "sched.h"
#include "shared.h"
#include "osenv.h"

static oskit_bus_t busi[N_FDC];
static oskit_blkdev_t devi[N_DRIVE];
#define devnode_to_drive(dev)	((dev) - &devi[0])

static oskit_error_t probe(struct driver_struct *ds);

/* Device driver structure representing the Linux floppy device driver */
static struct driver_struct drv = {
	{ &oskit_linux_driver_ops },
	probe,
	{ "floppy", "Linux floppy disk driver", NULL,
	  "Linus Torvalds & others", "Linux "UTS_RELEASE }
};

/* Descriptive information on a floppy controller */
static oskit_devinfo_t bus_info = {
	"floppy", "Floppy disk controller", NULL,
	"Linus Torvalds & others", "Linux "UTS_RELEASE
};

/* Descriptive information on a floppy disk drive */
static oskit_devinfo_t disk_info = {
	"disk", "Floppy disk drive of unknown type", NULL,
	"Linus Torvalds & others", "Linux "UTS_RELEASE
};


/*** Floppy disk device node interface ***/

static OSKIT_COMDECL
disk_query(oskit_blkdev_t *dev, const struct oskit_guid *iid, void **out_ihandle)
{
	if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
	    memcmp(iid, &oskit_device_iid, sizeof(*iid)) == 0 ||
	    memcmp(iid, &oskit_blkdev_iid, sizeof(*iid)) == 0) {
		*out_ihandle = dev;
		return 0;
	}

	*out_ihandle = NULL;
	return OSKIT_E_NOINTERFACE;
}

static OSKIT_COMDECL_U disk_addref(oskit_blkdev_t *dev)
{
	/* No reference counting for block device nodes */
	return 1;
}

static OSKIT_COMDECL_U disk_release(oskit_blkdev_t *dev)
{
	/* No reference counting for block device nodes */
	return 1;
}

static OSKIT_COMDECL disk_getinfo(oskit_blkdev_t *dev, oskit_devinfo_t *out_info)
{
	unsigned int drive = devnode_to_drive(dev);
	unsigned int type;

	*out_info = disk_info;

	type = drive_params[drive].cmos;
	if (type > 0 && type < NUMBER(default_drive_params))
		out_info->description = default_drive_params[type].name;

	return 0;
}

static OSKIT_COMDECL disk_getdriver(oskit_blkdev_t *fdev,
				    oskit_driver_t **out_driver)
{
	*out_driver = (oskit_driver_t*)&drv.drvi;
	drv.drvi.ops->addref(&drv.drvi);
	return 0;
}

static OSKIT_COMDECL disk_open(oskit_blkdev_t *dev, unsigned mode,
				oskit_blkio_t **out_blkio)
{
	kdev_t kdev = MKDEV(FLOPPY_MAJOR, devnode_to_drive(dev));

	return oskit_linux_block_open_kdev(kdev, NULL, mode, out_blkio);
}


static struct oskit_blkdev_ops disk_ops = {
	disk_query, disk_addref, disk_release,
	disk_getinfo, disk_getdriver,
	disk_open
};


/*** Floppy controller device node ("floppy bus") interface ***/

static OSKIT_COMDECL
bus_query(oskit_bus_t *bus, const struct oskit_guid *iid, void **out_ihandle)
{
	if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
	    memcmp(iid, &oskit_device_iid, sizeof(*iid)) == 0 ||
	    memcmp(iid, &oskit_bus_iid, sizeof(*iid)) == 0) {
		*out_ihandle = bus;
		return 0;
	}

	*out_ihandle = NULL;
	return OSKIT_E_NOINTERFACE;
}

static OSKIT_COMDECL_U
bus_addref(oskit_bus_t *bus)
{
	/* No reference counting for block device nodes */
	return 1;
}

static OSKIT_COMDECL_U
bus_release(oskit_bus_t *bus)
{
	/* No reference counting for block device nodes */
	return 1;
}

static OSKIT_COMDECL
bus_getinfo(oskit_bus_t *bus, oskit_devinfo_t *out_info)
{
	*out_info = bus_info;
	return 0;
}

static OSKIT_COMDECL
bus_getdriver(oskit_bus_t *bus, oskit_driver_t **out_driver)
{
	*out_driver = (oskit_driver_t*)&drv.drvi;
	drv.drvi.ops->addref(&drv.drvi);
	return 0;
}

static OSKIT_COMDECL
bus_getchild(oskit_bus_t *bus, oskit_u32_t idx,
	      struct oskit_device **out_fdev, char *out_pos)
{
	unsigned int fdc = bus - busi;
	unsigned int drive;

	osenv_assert(fdc < N_FDC);

	for (drive = REVDRIVE(fdc, 0); drive < REVDRIVE(fdc + 1, 0); ++drive) {
		if (drive_params[drive].cmos == 0 ||
		    !(allowed_drive_mask & (1 << drive)))
			continue;
		if (idx-- == 0) {
			/*
			 * Return the device node and
			 * position string for this drive.
			 */
			*out_fdev = (oskit_device_t*)&devi[drive];
			sprintf(out_pos, "drive%u", UNIT(drive));
			return 0;
		}
	}

	return OSKIT_E_DEV_NOMORE_CHILDREN;
}

static OSKIT_COMDECL
bus_probe(oskit_bus_t *bus)
{
	/*
	 * The floppy "bus" is probed when the controller itself is found,
	 * and dynamic insertion/removal isn't supported,
	 * so we don't have to do anything here.
	 */
	return 0;
}

static struct oskit_bus_ops bus_ops = {
	bus_query,
	bus_addref,
	bus_release,
	bus_getinfo,
	bus_getdriver,
	bus_getchild,
	bus_probe
};


/*** Driver initialization/probe code ***/

static oskit_error_t probe(struct driver_struct *ds)
{
	static int initialized = 0;
	oskit_error_t rc;
	int i, found;

	if (initialized)
		return 0;
	initialized = 1;

	/* Call the Linux floppy probe code */
	floppy_init();

	/*
	 * Register each floppy device found in the hardware tree.
	 * Each floppy controller becomes a "bus",
	 * and each device on an interface becomes a child of that bus.
	 */
	found = 0;
	for (i = 0; i < N_FDC; ++i) {

		/* Only interested in detected hardware interfaces */
		if (fdc_state[i].address == -1)
			continue;

		found++;

		/* Initialize and register the floppy controller */
		busi[i].ops = &bus_ops;
		rc = osenv_isabus_addchild(
			fdc_state[i].address,
			(oskit_device_t*)&busi[i]);
	}

	/* Initialize the device interfaces */
	for (i = 0; i < N_DRIVE; ++i) {

		/* Only interested in detected devices */
		if (drive_params[i].cmos == 0 ||
		    !(allowed_drive_mask & (1 << i)))
			continue;

		found++;

		devi[i].ops = &disk_ops;
	}

	return found;
}

oskit_error_t oskit_linux_init_floppy(void)
{
	return oskit_linux_driver_register(&drv);
}
