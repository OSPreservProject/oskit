/*
 * Copyright (c) 1996-2002 The University of Utah and the Flux Group.
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
 * 	NET3	Protocol independent device support routines.
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 */
/*
 * This code is called by the OS to request service from the driver.
 */

#include <oskit/c/assert.h>
#include <oskit/dev/error.h>
#include <oskit/dev/net.h>
#include <oskit/dev/ethernet.h>
#include <oskit/dev/native.h>

#ifdef HPFQ
#include <oskit/hpfq.h>
#endif

#include <linux/string.h>	/* memset */

#include "net_debug.h"

#include "net.h"
#include <linux/etherdevice.h>
#include "linux_emul.h"
#include "sched.h"
#include "osenv.h"
#define bzero(d,n) memset((d), 0, (n))

#ifndef OSKIT
#define OSKIT
#endif

struct device *dev_base;

/*
 * Forward declarations.
 */
static OSKIT_COMDECL linux_net_query(oskit_netio_t *io, const oskit_iid_t *iid,
				void **out_ihandle);
static OSKIT_COMDECL_U linux_net_addref(oskit_netio_t *io);
static OSKIT_COMDECL_U linux_net_release(oskit_netio_t *io);

static OSKIT_COMDECL linux_net_push(oskit_netio_t *ioi, oskit_bufio_t *b,
				    oskit_size_t size);
			   
static OSKIT_COMDECL linux_net_alloc_bufio(oskit_netio_t *ioi,
					   oskit_size_t size,
					   oskit_bufio_t **out_bufio);

/*** Network send I/O interface ***/

static struct oskit_netio_ops net_io_ops = {
	linux_net_query, linux_net_addref, linux_net_release,
	linux_net_push, linux_net_alloc_bufio
};


/*
 * Query a net I/O object for its interfaces.
 * This is extremely easy because we only export one interface
 * (plus its base type, IUnknown).
 */
static OSKIT_COMDECL
linux_net_query(oskit_netio_t *io, const oskit_iid_t *iid, void **out_ihandle)
{
	struct net_alias *dev = (struct net_alias*)
		((char*)io - SEND_IOI_OFS);

	if (dev == NULL)
		panic("%s:%d: null peropen", __FILE__, __LINE__);
	if (dev->send_ioi_count == 0)
		panic("%s:%d: bad count", __FILE__, __LINE__);

	if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
	    memcmp(iid, &oskit_netio_iid, sizeof(*iid)) == 0) {
		*out_ihandle = &dev->send_ioi;
		++dev->send_ioi_count;
		return 0;
	}

	*out_ihandle = NULL;
	return OSKIT_E_NOINTERFACE;
}

/*
 * Clone a reference to a device's block I/O interface.
 */
static OSKIT_COMDECL_U
linux_net_addref(oskit_netio_t *io)
{
	struct net_alias *dev = (struct net_alias*)
		((char*)io - SEND_IOI_OFS);

	if (dev == NULL)
		panic("%s:%d: null peropen", __FILE__, __LINE__);
	if (dev->send_ioi_count == 0)
		panic("%s:%d: bad count", __FILE__, __LINE__);

	return ++dev->send_ioi_count;
}


/*
 * Close ("release") a device.
 */
static OSKIT_COMDECL_U
linux_net_release(oskit_netio_t *io)
{
	struct net_alias *dev = (struct net_alias*)
		((char*)io - SEND_IOI_OFS);
	unsigned newcount;
	struct task_struct ts;

	if (dev == NULL)
		panic("%s:%d: null peropen", __FILE__, __LINE__);
	if (dev->send_ioi_count == 0)
		panic("%s:%d: bad count", __FILE__, __LINE__);

	newcount = --dev->send_ioi_count;
	if (newcount == 0) {

		OSKIT_LINUX_CREATE_CURRENT(ts);

		/*
		 * Let Linux do its device closing stuff.
		 */
		dev_close(dev->ldev);

		/*
		 * Release our reference on the client's
		 * receive net_io interface.
		 */
		if (dev->recv_ioi) {
			oskit_netio_release(dev->recv_ioi);
			dev->recv_ioi = NULL;
		}

		OSKIT_LINUX_DESTROY_CURRENT();

	}

	return newcount;
}


/*
 * Queue a packet for transmission.
 *
 * Compare
 *	Linux	net/core/dev.c:dev_queue_xmit
 *	Mach4	linux_net.c:device_write
 */
static OSKIT_COMDECL
linux_net_push(oskit_netio_t *ioi, oskit_bufio_t *b, oskit_size_t size)
{
	struct net_alias *dev = (struct net_alias*)
		((char*)ioi - SEND_IOI_OFS);
	unsigned long flags;
	struct sk_buff *skb;
	struct sk_buff_head *list;
	int err, special = 0;
	struct task_struct ts;
	oskit_size_t actual;
	void *p;

	/*
	 * Check for our own SKB impl.
	 */
	if ((skb = skbuff_is_skbufio(b))) {
		skb->len = size;
		special  = 1;
		goto doit;
	}

	/*
	 * Try to map the packet into contiguous local memory.
	 * Since the map operation on net_io interfaces is optional,
	 * we have to use copyin instead if this doesn't work.
	 */
	err = oskit_bufio_map(b, &p, 0, size);

	/*
	 * Get an skbuff and copy our data into it.  
	 * If the map operation didn't work, then we need 
	 * to allocate enough space to hold the packet.
	 *
	 * XXX should there be a size or alignment constraints?
	 */
	skb = alloc_skb(err ? size : 0, GFP_ATOMIC);

	if (skb == NULL) {
		if (!err)
			oskit_bufio_unmap(b, p, 0, size);

		return OSKIT_E_OUTOFMEMORY;
	}

	if (!err) {
		/* grab the reference first! */
		oskit_bufio_addref(b);
		assert(skb->data_io == NULL);
		skb->data_io = b;

		/* XXX This should probably be in a macro or something. */
		skb->data = p;
		skb->len = size;

		skb->head = skb->tail = skb->data;
                skb->end = skb->data + size;
	} else {
		/* Couldn't map, try to copy it in */
	        err = oskit_bufio_read(b, skb_put(skb, size), 0, size, &actual);
		assert(actual == size);
	}

	assert(err == 0);
 doit:
	current = &ts;	/* restore current after possibly blocking */

	/*
	 * First, try to send it right away.
	 */
#ifdef HPFQ
	/* The dev shouldn't be busy when we are HPFQ-ing. */
	if (oskit_pfq_root)
		assert(! dev->ldev->tbusy);
#endif

	if (dev->ldev->hard_start_xmit(skb, dev->ldev) == 0) {
		return 0;
	}

	/*
	 * For now, we have to ensure that our special SKBs are not
	 * queued here, unless of course we clone it to make sure
	 * that the same SKB is not queued more than once (thus messing
	 * up the queues).
	 */
	if (special && skb->list)
		return OSKIT_E_FAIL;		/* drop it */
	
	/*
	 * Send failed, e.g. device busy, queue it and schedule a
	 * software intr.
	 *
	 * (Real Linux has a race here, they non-atomically check
	 * the queue len and add to it.)
	 */
	flags = linux_save_flags_cli();
	list = &dev->ldev->buffs[0];
	if (skb_queue_len(list) > dev->ldev->tx_queue_len) {
		/* No room. */
		dev_kfree_skb(skb);
		linux_restore_flags(flags);
		return OSKIT_E_FAIL;		/* drop it */
	}
	__skb_queue_tail(list, skb);
	mark_bh(NET_BH);
	linux_restore_flags(flags);
	DPRINTF(D_SEND, "queued packet of len %ld on %s\n",
		skb->len, dev->ldev->name);

	return 0;
}

/*
 * Allocate a bufio in our preferred implementation.
 */
static OSKIT_COMDECL
linux_net_alloc_bufio(oskit_netio_t *io, oskit_size_t size,
		      oskit_bufio_t **out_bufio)
{
	struct net_alias *dev = (struct net_alias*)
		((char*)io - SEND_IOI_OFS);
	struct sk_buff *skb;

	osenv_assert(dev);
	osenv_assert(dev->send_ioi_count);

	skb = alloc_skb(size, GFP_ATOMIC);
	if (!skb)
		return OSKIT_E_OUTOFMEMORY;
	skb_put(skb, size);

	*out_bufio = (oskit_bufio_t *) &skb->ioi;

	return 0;
}


/*** Network device node interface methods ***/
/*
 * These functions are only default implementations;
 * network type-specific code (e.g., ethernet.c)
 * must provide the query method and type-specific methods.
 */

OSKIT_COMDECL_U
oskit_linux_netdev_addref(oskit_netdev_t *intf)
{
	/* No reference counting */
	return 1;
}

OSKIT_COMDECL_U
oskit_linux_netdev_release(oskit_netdev_t *intf)
{
	/* No reference counting */
	return 1;
}

OSKIT_COMDECL
oskit_linux_netdev_getinfo(oskit_netdev_t *intf, oskit_devinfo_t *out_info)
{
	struct net_alias *dev = (struct net_alias*)
		((char*)intf - DEVI_OFS);
	*out_info = dev->drv->dev_info;
	return 0;
}

OSKIT_COMDECL
oskit_linux_netdev_getdriver(oskit_netdev_t *intf, oskit_driver_t **out_driver)
{
	struct net_alias *dev = (struct net_alias*)
		((char*)intf - DEVI_OFS);
	oskit_driver_t *drvi = (oskit_driver_t*)&dev->drv->ds.drvi;

	*out_driver = drvi;
	oskit_driver_addref(drvi);

	return 0;
}

OSKIT_COMDECL
oskit_linux_netdev_open(oskit_netdev_t *intf, unsigned flags,
	   oskit_netio_t *recv_net_io,
	   oskit_netio_t **out_send_net_io)
{
	struct net_alias *dev = (struct net_alias*)
		((char*)intf - DEVI_OFS);
	struct task_struct ts;

	/* Only allow one opener at a time */
	if (dev->send_ioi_count > 0)
		return OSKIT_EBUSY;

#ifdef  DEVICE_POLLING
	/*
	 * Make sure the device supports polling.
	 */
	if ((flags & (OSKIT_ETHERDEV_RX_POLLING|OSKIT_ETHERDEV_TX_POLLING)) &&
	    dev->ldev->setup_polling == NULL)
		return OSKIT_E_NOTIMPL;
#endif
	

	OSKIT_LINUX_CREATE_CURRENT(ts);

	/*
	 * Try opening the thing.
	 * dev->open won't be set if not probed yet.
	 * This probing is done by the dev->init function,
	 * called by net_dev_init, which should be called before us by
	 * perhaps device_setup.
	 *
	 * XXX Both the Linux and Mach incarnations of this function allow
	 * dev->open to be NULL, I don't know why.
	 * Maybe some devs don't have open routines.
	 */
	assert(dev->ldev->open != NULL);
	if ((*dev->ldev->open)(dev->ldev) != 0) {
		DPRINTF(D_VINIT, "open for %s failed\n", dev->ldev->name);
		OSKIT_LINUX_DESTROY_CURRENT();
		return OSKIT_E_DEV_NOSUCH_DEV;
	}
	dev->ldev->flags |= (IFF_UP | IFF_RUNNING);

#ifdef  DEVICE_POLLING
	/*
	 * Turn on polling if that was requested.
	 */
	{
		int tx, rx;

		tx = rx = 0;
		if (flags & OSKIT_ETHERDEV_RX_POLLING) {
			rx = 1;
			dev->openflags |= OSKIT_ETHERDEV_RX_POLLING;
		}
		if (flags & OSKIT_ETHERDEV_TX_POLLING) {
			tx = 1;
			dev->openflags |= OSKIT_ETHERDEV_TX_POLLING;
		}
		if (tx || rx)
			(*dev->ldev->setup_polling)(dev->ldev, rx, tx);
	}
#endif

	OSKIT_LINUX_DESTROY_CURRENT();

	/*
	 * These conditions should hold if skb_queue_head_init has been run,
	 * which should have been done by net_dev_init.
	 */
	assert(dev->ldev->buffs->prev == (struct sk_buff *)dev->ldev->buffs);
	assert(dev->ldev->buffs->next == (struct sk_buff *)dev->ldev->buffs);
	assert(dev->ldev->buffs->qlen == 0);

	/* Stash a reference to the supplied recv_net_io interface */
	dev->recv_ioi = recv_net_io;
	if (dev->recv_ioi)
		oskit_netio_addref(recv_net_io);

	/* Caller starts out with one reference to the send_net_io interface */
	dev->send_ioi_count = 1;
	*out_send_net_io = &dev->send_ioi;
	return 0;			/* we opened the thing */
}

OSKIT_COMDECL
oskit_linux_netdev_rxpoll(oskit_netdev_t *intf,
			  int *count, oskit_bufio_t *out_bufios[])
{
#ifdef DEVICE_POLLING
	struct net_alias *dev = (struct net_alias*)
		((char*)intf - DEVI_OFS);

	osenv_assert(dev);
	osenv_assert(dev->send_ioi_count);

	if (dev->openflags & OSKIT_ETHERDEV_RX_POLLING) {
		struct sk_buff	*skbs[*count];
		int		i, got;

		got = (*dev->ldev->rx_poll)(dev->ldev, *count, skbs);

		if (got == 0) {
			*count = 0;
			return OSKIT_ENODATA;
		}

		/*
		 * Convert to oskit_bufio_t. Note that we give up our
		 * reference(s) to the caller, who must take care to
		 * release them when done.
		 */
		for (i = 0; i < got; i++)
			out_bufios[i] = (oskit_bufio_t *) &(skbs[i]->ioi);

		*count = got;
		return 0;
	}

	/*
	 * XXX hack: polling on a TX netio reaps any completed transmissions
	 * (i.e., releases the skbuffs).
	 */
	if (out_bufios == 0 && (dev->openflags & OSKIT_ETHERDEV_TX_POLLING)) {
		int got;

		got = (*dev->ldev->rx_poll)(dev->ldev, 0, 0);
		return got;
	}
#endif
	return OSKIT_ENODEV;
}



/*** Back door entrypoints to the Linux device drivers ***/

/*
 * Find the Linux network device named NAME.
 */
oskit_error_t
oskit_linux_netdev_find(const char *name, oskit_netdev_t **out_netdev)
{
	struct device *dev;
	oskit_netdev_t *devi;

	/*
	 * Find the dev struct corresponding to NAME.
	 * Linux's dev_get does something similar.
	 */
	for (dev = dev_base; dev != NULL; dev = dev->next) {
		DPRINTF(D_VINIT, "checking %s\n", dev->name);
		if (strcmp (name, dev->name) == 0)
			break;
	}
	if (dev == NULL) {
		DPRINTF(D_VINIT, "didn't find entry for %s\n", name);
		return OSKIT_E_DEV_NOSUCH_DEV;
	}

	/* Return a reference to this device node interface */
	*out_netdev = devi = &dev->my_alias->devi;
	oskit_netdev_addref(devi);
	return 0;
}


/*
 * Open the device named NAME.  In Linux this is called when someone
 * ups an interface with ifconfig.
 *
 * XXX the oskit docs don't specify open/closeity.
 *
 * Compare:
 *	Linux	net/core/dev.c:dev_open (and where it is called in dev_ifsioc.)
 *	Mach4	linux_dev.c:device_open
 */
oskit_error_t
oskit_linux_net_open(const char *name, unsigned flags, oskit_netio_t *in_io, 
	       oskit_netio_t **out_io)
{
	oskit_netdev_t *dev;
	oskit_error_t rc;

	/*
	 * Find the device node corresponding to NAME.
	 * Linux's dev_get does something similar.
	 */
	rc = oskit_linux_netdev_find(name, &dev);
	if (rc)
		return rc;

	/*
	 * Pass the call on to the device node's open routine.
	 */
	rc = oskit_netdev_open(dev, flags, in_io, out_io);
	oskit_netdev_release(dev);
	return rc;
}


/*** Standard probe routine for Linux network drivers ***/
/*
 * The probe pointer in the driver_struct (see glue.h)
 * for network drivers generally points to the wrapped
 * version of this routine (see next function).
 * It gets called by the probe method
 * in the driver node's COM interface (see driver.c),
 * and in turn calls the Linux driver's probe routine
 * after creating a device node for it to use.
 */
oskit_error_t
oskit_linux_netdev_probe_raw(struct driver_struct *ds)
{
	struct net_driver *drv = (struct net_driver*)ds;
	struct device *ldev;
	int found = 0;
	int rc;

	oskit_linux_net_init();

	osenv_log(OSENV_LOG_DEBUG, "Probing %s\n", ds->info.name);

	/*
	 * Probe for devices of this type repeatedly
	 * until we don't find any more.
	 */
	do {
		struct device **dp;
		int devnum;

		/*
		 * Create a "blank" Linux network device structure.
		 * Half of the Linux device drivers just ignore this
		 * and cons up their own structure,
		 * in some cases using init_etherdev() (e.g., tulip),
		 * in other cases just with a kmalloc (e.g., ewrk3);
		 * the other half use the structure you pass
		 * and expect it to be valid.  Ick!
		 */
		ldev = kmalloc(sizeof(*ldev) + 8, GFP_KERNEL);
		if (ldev == NULL)
			return OSKIT_E_OUTOFMEMORY;
		memset(ldev, 0, sizeof(*ldev));
		ldev->name = (char*)(ldev + 1);

		/*
		 * Cook up an appropriate Linux name for this device,
		 * and hook it into the device chain.
		 */
		devnum = 0;
		retry:
		sprintf(ldev->name, "%s%d", drv->basename, devnum);
		for (dp = &dev_base; *dp; dp = &(*dp)->next)
			if (strcmp(ldev->name, (*dp)->name) == 0) {
				devnum++;
				goto retry;
			}
		*dp = ldev;

		/*
		 * Call the Linux device driver's probe routine.
		 */
		rc = drv->probe(ldev);

		/*
		 * If our device struct wasn't used,
		 * then unhook it from the chain and free it.
		 */
		if (ldev->open == NULL) {
			assert(*dp == ldev);
			*dp = ldev->next;
			kfree(ldev);
		}

	} while (rc == 0);

	/*
	 * Now search the device list for devices that are alive
	 * but haven't yet had a net_alias structure attached,
	 * which means they aren't yet exported into the device tree
	 * and need to be.
	 */
	for (ldev = dev_base; ldev; ldev = ldev->next) {
		struct net_alias *dev;

		if (ldev->my_alias != NULL)
			continue;

		assert(ldev->name != NULL);
		assert(ldev->open != NULL);

		/*
		 * Found a new adaptor -
		 * create a corresponding net_alias struct.
		 */
		dev = kmalloc(sizeof(*dev), GFP_KERNEL);
		if (dev == NULL)
			return OSKIT_E_OUTOFMEMORY;
		ldev->my_alias = dev;
		dev->ldev = ldev;
		dev->drv = drv;
		dev->devi.ops = drv->dev_ops;
		dev->send_ioi.ops = &net_io_ops;
		dev->send_ioi_count = 0;
		dev->recv_ioi = NULL;
		dev->openflags = 0;

		/*
		 * Register the fdev device node
		 * in the device tree under the ISA bus.
		 */
		osenv_isabus_addchild(ldev->base_addr,
				  (oskit_device_t*)&dev->devi);

		/*
		 * Register the device node according to its interfaces.
		 */
		osenv_device_register((oskit_device_t*)&dev->devi,
				     dev->drv->dev_iids, dev->drv->dev_niids);

		found++;
	}

	return found;
}

#ifndef KNIT
/* locks already taken by standard startup code */
#define osenv_process_lock()
#define osenv_process_unlock()
#endif

oskit_error_t
oskit_linux_netdev_probe(struct driver_struct *ds)
{
        oskit_error_t rc;
        osenv_process_lock();
        rc = oskit_linux_netdev_probe_raw(ds);
        osenv_process_unlock();
        return rc;
}
