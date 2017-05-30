/*
 * Copyright (c) 1997- 2000 The University of Utah and the Flux Group.
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
 * This source file overrides the Linux block/ide.c,
 * but #include's the original ide.c as the first thing -
 * in other words, we basically extend Linux's ide.c at the bottom.
 */
#include "../drivers/block/ide.c"

#include <linux/version.h>

#include <oskit/dev/blk.h>
#include <oskit/dev/linux.h>
#include <oskit/dev/bus.h>
#include <oskit/dev/isa.h>
#include <oskit/dev/ide.h>
#include <oskit/dev/native.h>

#include "glue.h"
#include "block.h"
#include "osenv.h"

static oskit_error_t probe(struct driver_struct *ds);

/* Device driver structure representing the Linux IDE device driver */
static struct driver_struct drv = {
	{ &oskit_linux_driver_ops },
	probe,
	{ "ide", "Linux IDE device driver", NULL,
	  "Linus Torvalds & others", "Linux "UTS_RELEASE }
};

/* Descriptive information on an IDE hardware interface */
static oskit_devinfo_t bus_info = {
	"ide", "IDE bus interface", NULL,
	"Linus Torvalds & others", "Linux "UTS_RELEASE
};

/* Descriptive information for specific IDE devices */
static oskit_devinfo_t disk_info = {
	"disk", "IDE hard disk drive", NULL,
	"Linus Torvalds & others", "Linux "UTS_RELEASE
};
#if 0 /* XXX check the author names before using */
static oskit_devinfo_t tape_info = {
	"tape", "IDE tape drive", NULL,
	"Linus Torvalds & others", "Linux "UTS_RELEASE
};
static oskit_devinfo_t cdrom_info = {
	"cdrom", "IDE CD-ROM drive", NULL,
	"Linus Torvalds & others", "Linux "UTS_RELEASE
};
#endif

static oskit_blkdev_t devi[MAX_HWIFS][MAX_DRIVES];
static oskit_idebus_t busi[MAX_HWIFS];


/* Find the Linux ide_drive structure for a given device node */
static inline ide_drive_t *
devnode_to_drive(oskit_blkdev_t *dev)
{
	int idx;

	/* Find the index number in the devi array */
	idx = dev - &devi[0][0];
	if (idx < 0 || idx >= MAX_HWIFS*MAX_DRIVES)
		panic("%s:%d: bad blkdev pointer", __FILE__, __LINE__);

	return &ide_hwifs[idx / MAX_DRIVES].drives[idx % MAX_DRIVES];
}

/* Find the Linux device number for a given oskit_blkdev_t device node pointer */
static inline kdev_t
devnode_to_devnum(oskit_blkdev_t *dev)
{
	int idx, major, minor;

	/* Find the index number in the devi array */
	idx = dev - &devi[0][0];
	if (idx < 0 || idx >= MAX_HWIFS*MAX_DRIVES)
		panic("%s:%d: bad blkdev pointer", __FILE__, __LINE__);

	/* Derive the Linux major and minor numbers */
	major = ide_hwif_to_major[idx / MAX_DRIVES];
	minor = (idx % MAX_DRIVES) << PARTN_BITS;

	return MKDEV(major, minor);
}


/*** IDE disk device node interface ***/

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
	ide_drive_t *drive = devnode_to_drive(dev);

	*out_info = disk_info;

	/* If more detailed information is available, use it */
	if (drive->id && drive->id->model[0])
		out_info->description = drive->id->model;

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
	kdev_t kdev = devnode_to_devnum(dev);

	return oskit_linux_block_open_kdev(kdev, NULL, mode, out_blkio);
}


static struct oskit_blkdev_ops disk_ops = {
	disk_query, disk_addref, disk_release,
	disk_getinfo, disk_getdriver,
	disk_open
};


/*** IDE hardware interface device node ("IDE bus") interface ***/

static OSKIT_COMDECL
bus_query(oskit_idebus_t *bus, const struct oskit_guid *iid, void **out_ihandle)
{
	if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
	    memcmp(iid, &oskit_device_iid, sizeof(*iid)) == 0 ||
	    memcmp(iid, &oskit_bus_iid, sizeof(*iid)) == 0 ||
	    memcmp(iid, &oskit_idebus_iid, sizeof(*iid)) == 0) {
		*out_ihandle = bus;
		return 0;
	}

	*out_ihandle = NULL;
	return OSKIT_E_NOINTERFACE;
}

static OSKIT_COMDECL_U
bus_addref(oskit_idebus_t *bus)
{
	/* No reference counting for block device nodes */
	return 1;
}

static OSKIT_COMDECL_U
bus_release(oskit_idebus_t *bus)
{
	/* No reference counting for block device nodes */
	return 1;
}

static OSKIT_COMDECL
bus_getinfo(oskit_idebus_t *bus, oskit_devinfo_t *out_info)
{
	*out_info = bus_info;
	return 0;
}

static OSKIT_COMDECL
bus_getdriver(oskit_idebus_t *bus, oskit_driver_t **out_driver)
{
	*out_driver = (oskit_driver_t*)&drv.drvi;
	drv.drvi.ops->addref(&drv.drvi);
	return 0;
}

static OSKIT_COMDECL
bus_getchild(oskit_idebus_t *bus, oskit_u32_t idx,
	      struct oskit_device **out_fdev, char *out_pos)
{
	int hwif = bus - busi;
	int drive;

	if (hwif < 0 || hwif >= MAX_HWIFS)
		panic("%s:%d: bad bus pointer", __FILE__, __LINE__);

	/*
	 * Find the IDE device corresponding to this index.
	 * It may be somewhat unnecessary to go to this trouble
	 * since IDE may never support more than two drives per hwif
	 * and we're unlikely to find a slave without a master;
	 * but just for good practice...
	 */
	for (drive = 0; ; drive++) {
		if (drive >= MAX_DRIVES)
			return OSKIT_E_DEV_NOMORE_CHILDREN;
		if (!ide_hwifs[hwif].drives[drive].present)
			continue;
		if (idx-- == 0)
			break;
	}

	/* Return the device node and position string for this drive */
	*out_fdev = (oskit_device_t*)&devi[hwif][drive];
	strcpy(out_pos, drive ? "slave" : "master");
	return 0;
}

static OSKIT_COMDECL
bus_probe(oskit_idebus_t *bus)
{
	/*
	 * The IDE bus is probed when the IDE bus itself is found,
	 * and dynamic insertion/removal isn't supported,
	 * so we don't have to do anything here.
	 */
	return 0;
}

static struct oskit_idebus_ops bus_ops = {
	bus_query, 
	bus_addref, 
	bus_release,
	bus_getinfo, 
	bus_getdriver,
	bus_getchild, 
	bus_probe
};


/*** Driver initialization/probe code ***/

static oskit_error_t probe_raw(struct driver_struct *ds);

static oskit_error_t probe_raw(struct driver_struct *ds)
{
	static int initialized = 0;
	oskit_error_t rc;
	int i, j, found;

	if (initialized)
		return 0;
	initialized = 1;

	/* Call the Linux IDE probe code */
	ide_init();

	/*
	 * Register each IDE device found in the hardware tree.
	 * Each hardware interface becomes an IDE bus,
	 * and each device on an interface becomes a child of that bus.
	 */
	found = 0;
	for (i = 0; i < MAX_HWIFS; i++) {

		/* Only interested in detected hardware interfaces */
		if (blkdevs[ide_hwif_to_major[i]].fops == NULL)
			continue;
		found++;

		/* Initialize the device interfaces */
		for (j = 0; j < MAX_DRIVES; j++) {

			/* Only interested in detected devices */
			if (!ide_hwifs[i].drives[j].present)
				continue;

			if (ide_hwifs[i].drives[j].media == ide_tape) {
				osenv_log(OSENV_LOG_WARNING, 
					 "%s: IDE tape device %d not supported\n",
					 j, __FUNCTION__);

				ide_hwifs[i].drives[j].present = 0;
				continue;
			}

			found++;

			/* Use the appropriate operations vector table */
			switch (ide_hwifs[i].drives[j].media) {
			case ide_disk: devi[i][j].ops = &disk_ops; break;
			case ide_floppy: devi[i][j].ops = &disk_ops; break;
			case ide_cdrom: devi[i][j].ops = &disk_ops; break;
			}

			/* Perform linux per-drive boot time init */
			ide_revalidate_disk(MKDEV(ide_hwifs[i].major,
						  j<<PARTN_BITS));
		}

		/* Initialize and register the IDE bus */
		busi[i].ops = &bus_ops;
		rc = osenv_isabus_addchild(
			ide_hwifs[i].io_ports[IDE_DATA_OFFSET],
			(oskit_device_t*)&busi[i]);
	}

	return found;
}

#ifndef KNIT
/* locks already taken by standard startup code */
#define osenv_process_lock()
#define osenv_process_unlock()
#endif

static oskit_error_t probe(struct driver_struct *ds)
{
        oskit_error_t rc;
        osenv_process_lock();
        rc = probe_raw(ds);
        osenv_process_unlock();
        return rc;
}

oskit_error_t oskit_linux_init_ide(void)
{
	return oskit_linux_driver_register(&drv);
}

