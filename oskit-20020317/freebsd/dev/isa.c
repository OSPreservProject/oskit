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

#include <sys/param.h>
#include <sys/devconf.h>
#include <sys/interrupt.h>

#include <oskit/com.h>
#include <oskit/dev/tty.h>
#include <oskit/dev/freebsd.h>
#include <oskit/dev/native.h>

#include <oskit/dev/dev.h>
#include <oskit/c/assert.h>

#include "glue.h"
#include "isa_glue.h"

struct kern_devconf kdc_isa0 = {
	0, 0, 0,		/* filled in by dev_attach */
	"isa", 0, { MDDT_BUS, 0 },
	0, 0, 0, BUS_EXTERNALLEN,
	0 /* &kdc_cpu0 */,	/* parent is the CPU */
	0,			/* no parentdata */
	DC_BUSY,		/* busses are always busy */
	"ISA bus",
	DC_CLS_BUS		/* class */
};

static struct glue_isa_driver *gdrvs[ISA_DEVICE_MAX - ISA_DEVICE_MIN];
static oskit_ttydev_t devis[ISA_DEVICE_MAX - ISA_DEVICE_MIN];

int
isa_generic_externalize(struct proc *p, struct kern_devconf *kdc,
			void *userp, size_t l)
{
	return EINVAL;
}

int
isa_irq_pending(dvp)
	struct isa_device *dvp;
{
	unsigned id_irq;
	int irqno;

	id_irq = dvp->id_irq;
	for (irqno = 0; id_irq > 1; irqno++, id_irq >>= 1);
	return osenv_irq_pending(irqno);
}

static void irqhandler(void *data)
{
	struct isa_device *dev = (struct isa_device*)data;

	unsigned cpl;

	save_cpl(&cpl);
	assert(osenv_intr_enabled() == 0);
	splhigh();

	/* Invoke the actual driver's interrupt routine */
	dev->id_intr(dev->id_unit);

	assert(osenv_intr_enabled() == 0);
	restore_cpl(cpl);

	swi_check();
}


/*** ISA Device Interface ***/
/*
 * Currently this interface is specific to character (tty) devices;
 * to add block device support this'll need to be split up a little.
 */

static OSKIT_COMDECL
dev_query(oskit_ttydev_t *devi, const struct oskit_guid *iid,
	  void **out_ihandle)
{
	if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
	    memcmp(iid, &oskit_driver_iid, sizeof(*iid)) == 0 ||
	    memcmp(iid, &oskit_ttydev_iid, sizeof(*iid)) == 0) {
		*out_ihandle = devi;
		return 0;
	}

	*out_ihandle = 0;
	return OSKIT_E_NOINTERFACE;
}

static OSKIT_COMDECL_U
dev_add_ref(oskit_ttydev_t *devi)
{
	/* No reference counting for static device nodes */
	return 1;
}

static OSKIT_COMDECL_U
dev_release(oskit_ttydev_t *devi)
{
	/* No reference counting for static device nodes */
	return 1;
}

static OSKIT_COMDECL
dev_get_info(oskit_ttydev_t *devi, oskit_devinfo_t *out_info)
{
	int idx = devi - devis;

	*out_info = gdrvs[idx]->dev_info;
	return 0;
}

static OSKIT_COMDECL
dev_get_driver(oskit_ttydev_t *devi, oskit_driver_t **out_driver)
{
	int idx = devi - devis;

	*out_driver = (oskit_driver_t*)&gdrvs[idx]->drvi;
	return 0;
}

static OSKIT_COMDECL
dev_open(oskit_ttydev_t *devi, oskit_u32_t flags,
	 struct oskit_ttystream **out_ttystream)
{
	int idx = devi - devis;
	oskit_error_t rc;

	rc = oskit_freebsd_chardev_open(gdrvs[idx]->major,
				    isa_devtab_tty[idx].id_unit | 0x80,
				    flags, out_ttystream);
	if (rc == OSKIT_ENXIO)
		return oskit_freebsd_chardev_open(gdrvs[idx]->major,
					      isa_devtab_tty[idx].id_unit,
					      flags, out_ttystream);
	return rc;
}

static OSKIT_COMDECL
dev_listen(oskit_ttydev_t *devi, oskit_u32_t flags,
	   struct oskit_ttystream **out_ttystream)
{
	int idx = devi - devis;

	return oskit_freebsd_chardev_open(gdrvs[idx]->major,
				      isa_devtab_tty[idx].id_unit,
				      flags, out_ttystream);
}

static struct oskit_ttydev_ops dev_ops = {
	dev_query, dev_add_ref, dev_release,
	dev_get_info, dev_get_driver,
	dev_open, dev_listen
};


/*** ISA Device Driver Interface ***/

static OSKIT_COMDECL
drv_query(oskit_isa_driver_t *drvi, const struct oskit_guid *iid,
	  void **out_ihandle)
{
	struct glue_isa_driver *gdrv = (struct glue_isa_driver*)drvi;

	if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
	    memcmp(iid, &oskit_driver_iid, sizeof(*iid)) == 0 ||
	    memcmp(iid, &oskit_isa_driver_iid, sizeof(*iid)) == 0) {
		*out_ihandle = &gdrv->drvi;
		return 0;
	}

	*out_ihandle = 0;
	return OSKIT_E_NOINTERFACE;
}

static OSKIT_COMDECL_U
drv_add_ref(oskit_isa_driver_t *drvi)
{
	/* No reference counting for static driver nodes */
	return 1;
}

static OSKIT_COMDECL_U
drv_release(oskit_isa_driver_t *drvi)
{
	/* No reference counting for static driver nodes */
	return 1;
}

static OSKIT_COMDECL
drv_get_info(oskit_isa_driver_t *drvi, oskit_devinfo_t *out_info)
{
	struct glue_isa_driver *gdrv = (struct glue_isa_driver*)drvi;

	*out_info = gdrv->drv_info;
	return 0;
}

static OSKIT_COMDECL
drv_probe(oskit_isa_driver_t *drvi, oskit_isabus_t *notused)
{
	struct glue_isa_driver *gdrv = (struct glue_isa_driver*)drvi;
	struct proc p;
	int found = 0;
	int i;
	int iosize;

	OSKIT_FREEBSD_CREATE_CURPROC(p);

	for (i = 0; isa_devtab_tty[i].id_driver; i++) {
		if (isa_devtab_tty[i].id_driver != gdrv->drv)
			continue;
		if (isa_devtab_tty[i].id_enabled == 0)
			continue;

		/*
		 * Don't probe if this I/O address is already taken.
		 * Unfortunately we can't check the whole region,
		 * because we won't know the region's size
		 * until _after_ a successful probe (thanks FreeBSD),
		 * so we only test the base I/O address itself.
		 */
		if (isa_devtab_tty[i].id_iobase > 0 &&
		    !osenv_io_avail(isa_devtab_tty[i].id_iobase, 1))
			continue;

		/* Probe for this device at this location */
		iosize = gdrv->drv->probe(&isa_devtab_tty[i]);
		if (iosize == 0)
			continue;

		/* Allocate the device's full I/O region. */
		if (isa_devtab_tty[i].id_iobase > 0 &&
		    osenv_io_alloc(isa_devtab_tty[i].id_iobase, iosize))
			continue;

		/*
		 * Register the interrupt handler for this driver.
		 */
		if (isa_devtab_tty[i].id_irq) {
			int irqno;

			for (irqno = 0;
			     (1 << irqno) != isa_devtab_tty[i].id_irq;
			     irqno++);
			if (osenv_irq_alloc(irqno, irqhandler,
					   &isa_devtab_tty[i], 0)) {
				/*
				 * This device's irq is already taken
				 * by something else, so the driver can't work.
				 */
				if (isa_devtab_tty[i].id_iobase > 0) {
					int ib = isa_devtab_tty[i].id_iobase;
					printf("fdev_freebsd: %s at io %x-%x"
					       " needs busy irq %d\n",
					       gdrv->dev_info.name,
					       ib, ib + iosize - 1,
					       irqno);
					/*
					 * Deallocate the io space we got.
					 */
					osenv_io_free(ib, iosize);
				}
				else
					printf("fdev_freebsd: "
					       "%s needs busy irq %d\n",
					       gdrv->dev_info.name, irqno);
				printf("fdev_freebsd: %s not attached\n",
				       gdrv->dev_info.description);
				continue;
			}
		}

		isa_devtab_tty[i].id_alive = iosize;

		/*
		 * The probe was successful, so attach the device.
		 * We just ignore the return value
		 * because that's what FreeBSD's config_isadev() does,
		 * and because the drivers don't return anything consistent.
		 */
		gdrv->drv->attach(&isa_devtab_tty[i]);

		/* Register this driver on the ISA bus node */
		gdrvs[i] = gdrv;
		devis[i].ops = &dev_ops;
		osenv_isabus_addchild(isa_devtab_tty[i].id_iobase, 
                                      (oskit_device_t*)&devis[i]);

		/* Register this device in the interface registry */
		osenv_device_register((oskit_device_t*)&devis[i],
					  &oskit_ttydev_iid, 1);

		found++;
	}

	OSKIT_FREEBSD_DESTROY_CURPROC(p);

	return found;
}

static struct oskit_isa_driver_ops drv_ops =
{
	drv_query, drv_add_ref, drv_release,
	drv_get_info, drv_probe
};


extern void oskit_freebsd_isa_init(void)
{
	static int inited;
	static struct isa_driver dummy_driver;
	int i;

	if (inited)
		return;
	inited = 1;

	osenv_isabus_init(); /*XXX*/

	oskit_freebsd_init();

	for (i = 0; i < ISA_DEVICE_MAX - ISA_DEVICE_MIN; i++) {
		isa_devtab_tty[i].id_id = ISA_DEVICE_MIN + i;
		isa_devtab_tty[i].id_driver = &dummy_driver;
		isa_devtab_tty[i].id_iobase = -1;
		isa_devtab_tty[i].id_irq = -1;
		isa_devtab_tty[i].id_drq = -1;
	}
}

extern oskit_error_t
oskit_freebsd_isa_register(struct glue_isa_driver *gdrv,
			  struct cdevsw *gcdevsw)
{
	oskit_error_t rc;

	/* Finish initializing the glue_isa_driver structure */
	gdrv->drvi.ops = &drv_ops;

	/* Fill in the appropriate entry of the cdevsw table */
	if (gdrv->major >= nchrdev)
		panic("cdevsw table too small!");
	cdevsw[gdrv->major] = *gcdevsw;

	/* Register this device driver */
	rc = osenv_driver_register((oskit_driver_t*)&gdrv->drvi,
				  &oskit_isa_driver_iid, 1);
	if (rc)
		return rc;

	return 0;
}

