/*
 * Copyright (c) 1996-2000 The University of Utah and the Flux Group.
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
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 */
/*
 * Definitions of things normally provided by Linux.
 */

#include <oskit/dev/net.h>
#include <oskit/io/bufio.h>
#include <oskit/io/netio.h>
#ifdef HPFQ
#include <oskit/hpfq.h>
#endif

#include <oskit/c/assert.h>

#include "net.h"
#include "net_debug.h"
#include "osenv.h"

#include <linux/skbuff.h>		/* struct sk_buff */
#include <linux/malloc.h>		/* kmalloc/free */
#include <linux/netdevice.h>		/* struct device */
#include <linux/interrupt.h>		/* NET_BH */
#include <linux/autoconf.h>		/* CONFIG_foo */
#include <linux/string.h>		/* memcmp */
#include <linux/rtnetlink.h>		/* shlock goo */

#include "irq.h"

static struct sk_buff_head backlog;	/* the receive queue */
static int backlog_size = 0;
const int backlog_max = 300;		/* wow, but that is what Linux has */

#ifdef BACKLOGSTATS
static int backlog_queued, backlog_dropped, backlog_maxlen;
#endif


/*
 * Initialize the Linux network glue module.
 * This roughly corresponds to Linux's net_dev_init(),
 * except it doesn't walk the static device list
 * because we don't have one!
 */
oskit_error_t
oskit_linux_net_init(void)
{
	static int initialized;

	if (initialized)
		return 0;
	initialized = 1;

	skb_queue_head_init(&backlog);
	init_bh(NET_BH, net_bh);

	return 0;
}


/*
 * Close the device DEV.
 *
 * Compare:
 *	Linux	net/core/dev.c
 *	Mach4	linux_net.c
 *
 * The Linux one calls the dev->stop routine and purges any queued packets.
 * That is what we do.
 * The Mach one does NOTHING!  We used to do that but it was a bad idea.
 */
int
dev_close(struct device *dev)
{
	int ct = 0;

	/*
	 *	Call the device specific close. This cannot fail.
	 *	Only if device is UP
	 */
	 
	if ((dev->flags & IFF_UP) && dev->stop)
		dev->stop(dev);

	/*
	 *	Device is now down.
	 */
	 
	dev->flags&=~(IFF_UP|IFF_RUNNING);

	/*
	 *	Purge any queued packets when we down the link 
	 */
	while(ct < DEV_NUMBUFFS)
	{
		struct sk_buff *skb;
		while((skb = skb_dequeue(&dev->buffs[ct])) != NULL)
			kfree_skb(skb);
		ct++;
	}

#ifdef BACKLOGSTATS
	printf("backlog: %d queued, %d dropped, %d (%d) max qlen\n",
	       backlog_queued, backlog_dropped, backlog_maxlen, backlog_max);
#endif

	return 0;
}


/* Software interrupts. */

/*
 * This is similar but not equivalent to Linux's dev_transmit,
 * therefore it is not named dev_transmit.
 */
static void
my_dev_transmit(void)
{
	int len;
	struct sk_buff *skb;
	struct device *dev;

	/* Start transmission on interfaces.  */
	for (dev = dev_base; dev != NULL; dev = dev->next) {
		/* Linux just checks for non-zero dev->flags. */
		if (!(dev->flags & IFF_UP) || dev->tbusy)
			continue;

#ifdef HPFQ
		if (oskit_pfq_root) {
			oskit_pfq_reset_path(oskit_pfq_root);
			continue;
		}
#endif

		DPRINTF(D_VSEND, "%s: trying to send\n", dev->name);
		for (;;) {
			skb = skb_dequeue(&dev->buffs[0]);
			if (skb == NULL) {
				DPRINTF(D_VSEND, "%s: nothing to send\n",
					dev->name);
				break;
			}
			len = skb->len;

			if ((*dev->hard_start_xmit)(skb, dev) != 0) {
				DPRINTF(D_VSEND, "%s: send failed, requeue\n",
					dev->name);
				skb_queue_head(&dev->buffs[0], skb);
				mark_bh(NET_BH);
				break;
			}
			else
				DPRINTF(D_SEND, "%s: sent pkt w/len %d\n",
					dev->name, len);
		}
	}
}

/*
 * The software interrupt handler, or bottom-half handler.
 * Its job is to start transmission on all interfaces and to
 * try to empty the receive queue.
 *
 * We are called with interrupts on (XXX make an assert()ion.)
 *
 * The Linux one makes sure it is called atomically but that is only
 * needed if this is NOT called thru the bottom-half mechanism.
 *
 *	Linux	net/core/dev.c
 *	Mach4	linux_net.c
 */
void
net_bh(void)
{
	struct sk_buff *skb;
	
	my_dev_transmit();

	/*
	 * Be careful not to break out of this loop strangely
	 * since we need the cli before dequeueing.
	 * No, we can't just use the non-underscore version of skb_dequeue
	 * since we need to serialize access to backlog_size too.
	 */
	linux_cli();
	while ((skb = __skb_dequeue(&backlog)) != NULL) {
		struct device *dev = skb->dev;
		oskit_error_t rc;

		backlog_size--;
		linux_sti();
		DPRINTF(D_RECV, "passing packet of len %ld to OS\n", skb->len);

		/*
		 * Device interrupts may be turned on in netdev_open
		 * (via the specific driver's open routine) before it
		 * has initialized recv_ioi.
		 */
		if (dev->my_alias->recv_ioi != NULL) {
			rc = oskit_netio_push(dev->my_alias->recv_ioi,
					      (oskit_bufio_t *)&skb->ioi,
					      skb->len);
			if (rc != 0) {
				DPRINTF(D_VRECV, "oskit_netio_push failed\n");
			}			
		} else {
			DPRINTF(D_VRECV, "oskit_netio_push no receiver\n");
		}
		oskit_bufio_release((oskit_bufio_t *) &skb->ioi);
		linux_cli();
	}
	linux_sti();

	my_dev_transmit();		/* supposedly a ood idea */
}


/* Hardware interrupts. */

/*
 * Accept packet SKB received on an interface and queue it.
 *
 * Compare
 *	Linux	net/core/dev.c
 *	Mach4	linux_net.c
 *
 * The Mach one copies it into a Mach ipc_kmsg_t, frees it, and calls
 * Mach's net_packet (which does queuing.)
 * The Linux one adds it to a backlog queue (which is emptied in the
 * bottom half handler), and then mark_bh(NET_BH)
 */
void
netif_rx(struct sk_buff *skb)
{
	static int dropping = 0;

	/*
	 *	Check that we aren't overdoing things.
	 */
	DPRINTF(D_RECV, "%s: recv packet of len %ld\n",
		skb->dev->name, skb->len);
	DPRINTF(D_VRECV, "backlog size %d\n", backlog_size);
	if (!backlog_size)
  		dropping = 0;
	else if (backlog_size > backlog_max)
		dropping = 1;

	if (dropping) 
	{
#ifdef BACKLOGSTATS
		backlog_dropped++;
#endif
		kfree_skb(skb);
		return;
	}

	/*
	 *	Add it to the "backlog" queue. 
	 */
#if CONFIG_SKB_CHECK
	IS_SKB(skb);
#endif	

	skb_queue_tail(&backlog, skb);
	backlog_size++;
#ifdef BACKLOGSTATS
	if (backlog_size > backlog_maxlen)
		backlog_maxlen = backlog_size;
	backlog_queued++;
#endif
  
	/*
	 *	If any packet arrived, mark it for processing after the
	 *	hardware interrupt returns.
	 */

#ifdef CONFIG_NET_RUNONIRQ	/* Dont enable yet, needs some driver mods */
	net_bh();
#else
	mark_bh(NET_BH);
#endif
	return;
}

atomic_t rtnl_rlockct;
struct wait_queue *rtnl_wait;

void rtnl_lock()
{
	rtnl_shlock();
	rtnl_exlock();
}

void rtnl_unlock()
{
	rtnl_exunlock();
	rtnl_shunlock();
}

void skb_over_panic(struct sk_buff *skb, int sz, void *here)
{
	panic("skput:over: %p:%ld put:%d dev:%s", 
		here, skb->len, sz, skb->dev ? skb->dev->name : "<NULL>");
}

int register_netdevice(struct device *dev)
{
	struct device *d, **dp;
	static int devindex;

	dev->iflink = -1;

	/* Init, if this function is available */
	if (dev->init && dev->init(dev) != 0)
		return -EIO;

	/* Check for existence, and append to tail of chain */
	for (dp=&dev_base; (d=*dp) != NULL; dp=&d->next) {
		if (d == dev || strcmp(d->name, dev->name) == 0)
			return -EEXIST;
	}
	dev->next = NULL;

	dev->ifindex = devindex++;
	if (dev->iflink == -1)
		dev->iflink = dev->ifindex;
	*dp = dev;

	return 0;
}

int unregister_netdevice(struct device *dev)
{
	struct device *d, **dp;

	if (1) {
		/* If device is running, close it.
		   It is very bad idea, really we should
		   complain loudly here, but random hackery
		   in linux/drivers/net likes it.
		 */
		if (dev->flags & IFF_UP)
			dev_close(dev);

#ifdef CONFIG_NET_FASTROUTE
		dev_clear_fastroute(dev);
#endif
	}

	/* And unlink it from device chain. */
	for (dp = &dev_base; (d=*dp) != NULL; dp=&d->next) {
		if (d == dev) {
			*dp = d->next;
			synchronize_bh();
			d->next = NULL;

			if (dev->destructor)
				dev->destructor(dev);
			return 0;
		}
	}
	return -ENODEV;
}

