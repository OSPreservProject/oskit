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

#include <oskit/io/blkio.h>
#include <oskit/dev/driver.h>
#include <oskit/dev/device.h>
#include <oskit/dev/bus.h>
#include <oskit/dev/blk.h>
#include <oskit/dev/scsi.h>
#include <oskit/dev/native.h>
#include <oskit/dev/linux.h>

#include "scsi_glue.h"
#include "sd.h"

#include <linux/module.h>

struct scsi_bus;

struct scsi_dev {
	oskit_device_t			intf;
	struct scsi_dev			*next;
	Scsi_Device			*dev;
	struct scsi_bus			*bus;
	char				model[16+1];
	char				vendor[8+1];
	kdev_t				devno;
};

struct scsi_bus {
	oskit_scsibus_t			intf;
	struct scsi_driver_struct	*sds;
	struct Scsi_Host		*host;
	struct scsi_dev			*devs;
};


/*** Generic (unrecognized) SCSI device node interface ***/

static oskit_devinfo_t dev_info = {
	"scsi", "Unknown SCSI device", NULL,
	"Drew Eckhardt, Eric Youngdale", "Linux "UTS_RELEASE
};

static OSKIT_COMDECL
dev_query(oskit_device_t *dev, const struct oskit_guid *iid, void **out_ihandle)
{
	if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
	    memcmp(iid, &oskit_device_iid, sizeof(*iid)) == 0) {
		*out_ihandle = dev;
		return 0;
	}

	*out_ihandle = NULL;
	return OSKIT_E_NOINTERFACE;
}

static OSKIT_COMDECL_U dev_addref(oskit_device_t *dev)
{
	/* No reference counting */
	return 1;
}

static OSKIT_COMDECL_U dev_release(oskit_device_t *dev)
{
	/* No reference counting */
	return 1;
}

static OSKIT_COMDECL dev_getinfo(oskit_device_t *dev, oskit_devinfo_t *out_info)
{
	struct scsi_dev *d = (struct scsi_dev*)dev;

	*out_info = dev_info;

	/* If more detailed information is available, use it */
	if (d->model[0]) out_info->description = d->model;
	if (d->vendor[0]) out_info->vendor = d->vendor;

	return 0;
}

static OSKIT_COMDECL dev_getdriver(oskit_device_t *dev,
				    oskit_driver_t **out_driver)
{
	struct scsi_dev *d = (struct scsi_dev*)dev;
	oskit_driver_t *drvi = (oskit_driver_t*)&d->bus->sds->ds.drvi;

	*out_driver = drvi;
	oskit_driver_addref(drvi);

	return 0;
}

static struct oskit_device_ops dev_ops = {
	dev_query, dev_addref, dev_release,
	dev_getinfo, dev_getdriver
};


/*** SCSI disk device node interface ***/

static oskit_devinfo_t disk_info = {
	"disk", "SCSI hard disk drive", NULL,
	"Drew Eckhardt, Eric Youngdale", "Linux "UTS_RELEASE
};

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

static OSKIT_COMDECL disk_getinfo(oskit_blkdev_t *dev, oskit_devinfo_t *out_info)
{
	struct scsi_dev *d = (struct scsi_dev*)dev;

	*out_info = disk_info;

	/* If more detailed information is available, use it */
	if (d->model[0]) out_info->description = d->model;
	if (d->vendor[0]) out_info->vendor = d->vendor;

	return 0;
}

static OSKIT_COMDECL disk_open(oskit_blkdev_t *dev, unsigned mode,
				oskit_blkio_t **out_blkio)
{
	struct scsi_dev *d = (struct scsi_dev*)dev;

	return oskit_linux_block_open_kdev(d->devno, NULL, mode, out_blkio);
}

static struct oskit_blkdev_ops disk_ops = {
	disk_query, (void*)dev_addref, (void*)dev_release,
	disk_getinfo, (void*)dev_getdriver,
	disk_open
};


/*** SCSI host adaptor device node ("SCSI bus") interface ***/

static OSKIT_COMDECL
bus_query(oskit_scsibus_t *bus, const struct oskit_guid *iid, void **out_ihandle)
{
	if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
	    memcmp(iid, &oskit_device_iid, sizeof(*iid)) == 0 ||
	    memcmp(iid, &oskit_bus_iid, sizeof(*iid)) == 0 ||
	    memcmp(iid, &oskit_scsibus_iid, sizeof(*iid)) == 0) {
		*out_ihandle = bus;
		return 0;
	}

	*out_ihandle = NULL;
	return OSKIT_E_NOINTERFACE;
}

static OSKIT_COMDECL_U
bus_addref(oskit_scsibus_t *bus)
{
	/* No reference counting */
	return 1;
}

static OSKIT_COMDECL_U
bus_release(oskit_scsibus_t *bus)
{
	/* No reference counting */
	return 1;
}

static OSKIT_COMDECL
bus_getinfo(oskit_scsibus_t *intf, oskit_devinfo_t *out_info)
{
	struct scsi_bus *bus = (struct scsi_bus*)intf;
	*out_info = bus->sds->dev_info;
	return 0;
}

static OSKIT_COMDECL
bus_getdriver(oskit_scsibus_t *intf, oskit_driver_t **out_driver)
{
	struct scsi_bus *bus = (struct scsi_bus*)intf;
	oskit_driver_t *drvi = (oskit_driver_t*)&bus->sds->ds.drvi;

	*out_driver = drvi;
	oskit_driver_addref(drvi);

	return 0;
}

static OSKIT_COMDECL
bus_getchild(oskit_scsibus_t *intf, oskit_u32_t idx,
	     struct oskit_device **out_fdev, char *out_pos)
{
	struct scsi_bus *bus = (struct scsi_bus*)intf;
	struct scsi_dev *dev;

	/* Find the SCSI device corresponding to this index */
	for (dev = bus->devs; ; dev = dev->next) {
		if (dev == NULL)
			return OSKIT_E_DEV_NOMORE_CHILDREN;
		if (idx-- == 0)
			break;
	}

	/* Return the device node and position string for this drive */
	*out_fdev = &dev->intf;
	sprintf(out_pos, "id%dlun%d", dev->dev->id, dev->dev->lun);
	return 0;
}

static OSKIT_COMDECL
bus_probe(oskit_scsibus_t *bus)
{
	/*
	 * The SCSI bus is probed when the SCSI host adaptor is found,
	 * and dynamic insertion/removal isn't supported,
	 * so we don't have to do anything here.
	 */
	return 0;
}

static struct oskit_scsibus_ops bus_ops = {
	bus_query,
	bus_addref,
	bus_release,
	bus_getinfo,
	bus_getdriver,
	bus_getchild,
	bus_probe
};

oskit_error_t
oskit_linux_scsi_probe(struct driver_struct *ds)
{
	struct scsi_driver_struct *sds = (struct scsi_driver_struct*)ds;
	struct Scsi_Host *host;
	oskit_error_t rc;
	int found;

	/*
	 * scsi_dev_init was already called by device_setup, which was
	 * called by oskit_linux_dev_init, which got called in
	 * oskit_linux_driver_register before we could be here.
	 *
	 * Initialize this driver if not done yet.
	 */
	if (sds->initialized)
		return 0;
	sds->initialized = 1;
	scsi_register_module(MODULE_SCSI_HA, &sds->tmpl);

	/* Add any detected SCSI host adaptors to the fdev device tree */
	found = 0;
	for (host = scsi_hostlist; host; host = host->next) {
		struct scsi_bus *bus;
		struct scsi_dev **devp;
		Scsi_Device *ldev;

		if (host->hostt != &sds->tmpl)
			continue;

		/* Create a device node for this host adaptor */
		/* XXX deal with multi-bus host adaptors */
		if ((bus = kmalloc(sizeof(*bus), 0)) == 0)
			return OSKIT_E_OUTOFMEMORY;
		bus->intf.ops = &bus_ops;
		bus->sds = sds;
		bus->host = host;
		bus->devs = NULL;
		rc = osenv_isabus_addchild(host->io_port,
				       (oskit_device_t*)bus);
		if (rc != 0)
			return rc;

		/* Create device sub-nodes for each of the devices found */
		devp = &bus->devs;
		for (ldev = host->host_queue; ldev; ldev = ldev->next) {
			struct scsi_dev *dev;
			int i;

			if (ldev->host != host)
				continue;

			/* Create a new device node */
			if ((dev = kmalloc(sizeof(*dev), 0)) == 0)
				return OSKIT_E_OUTOFMEMORY;
			dev->intf.ops = &dev_ops;
			dev->next = NULL;
			dev->dev = ldev;
			dev->bus = bus;

			/* Build the device information strings */
			memcpy(dev->model, ldev->model, 16);
			dev->model[16] = 0;
			for (i = 15; i >= 0; i--) {
				if (dev->model[i] > ' ')
					break;
				dev->model[i] = 0;
			}
			memcpy(dev->vendor, ldev->vendor, 8);
			dev->vendor[8] = 0;
			for (i = 7; i >= 0; i--) {
				if (dev->vendor[i] > ' ')
					break;
				dev->vendor[i] = 0;
			}

			/* See if we know about this particular device type */
			/* XXX would be better if the high-level drivers
			   could be separated out, later... */
			for (i = 0; i < sd_template.dev_max; i++) {
				if (rscsi_disks[i].device != ldev)
					continue;
				dev->intf.ops = (void*)&disk_ops;
				dev->devno = MKDEV(SCSI_DISK0_MAJOR, i << 4);
			}

			/* Hook this device onto the host adaptor's list */
			*devp = dev;
			devp = &dev->next;

			found++;
		}

		found++;
	}

	return found;
}

void
proc_print_scsidevice(Scsi_Device *scd, char *buffer, int *size, int len)
{
}
