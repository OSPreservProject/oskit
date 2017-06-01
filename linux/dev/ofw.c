/*
 * Copyright (c) 1996-1999 The University of Utah and the Flux Group.
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

/*	$NetBSD: ofbus.c,v 1.10 1998/02/24 05:44:39 mycroft Exp $	*/

/*
 * Copyright (C) 1995, 1996 Wolfgang Solfrank.
 * Copyright (C) 1995, 1996 TooLs GmbH.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifdef OSKIT_ARM32_SHARK
/*
 * OFW glue code. This code requires a couple of routines in the kernel
 * library to speak to the actual OFW interfaces.
 */

#include <oskit/types.h>
#include <oskit/arm32/ofw/ofw.h>

#include <linux/string.h>
#include <linux/kernel.h>

#if 0
#define DPRINTF(fmt, args... ) printk(__FUNCTION__  ":" fmt , ## args)
#else
#define DPRINTF(fmt, args... )
#endif

/*
 * Debugging code to print the entire OFW device tree.
 */
void
oskit_linux_ofw_nodedump(int phandle, int level)
{
	int	child, i;
	char	name[32];

	for (i = 0; i < level; i++)
		printk(" ");

	if (OF_getprop(phandle, "name", name, sizeof(name)) > 0) {
		printk("0x%x:%s\n", phandle, name);
	}
	else {
		printk("0x%x:????", phandle);
	}

	for (child = OF_child(phandle); child; child = OF_peer(child)) {
		oskit_linux_ofw_nodedump(child, level + 1);
	}
}

void
oskit_linux_ofw_busdump(void)
{
	int	node;
	int	child;

	if (!(node = OF_peer(0)))
		panic("No OFW root");

	for (child = OF_child(node); child; child = OF_peer(child)) {
		oskit_linux_ofw_nodedump(child, 0);
	}
}

/*
 * Search the OFW's device tree for a named node, and return the package
 * handle for it.
 */
int
oskit_linux_ofw_findnode(char *name)
{
	int	node, child;
	int	oskit_linux_ofw_findnode_child(char *, int);

	if (!(node = OF_peer(0)))
		panic("No OFW root");

	for (child = OF_child(node); child; child = OF_peer(child)) {
		if ((node = oskit_linux_ofw_findnode_child(name, child)))
			return node;
	}

	return 0;
}

/*
 * Aux routine for above.
 */
int
oskit_linux_ofw_findnode_child(char *nodename, int phandle)
{
	int	node, child;
	char	name[32];

	if (OF_getprop(phandle, "name", name, sizeof(name)) > 0) {
		if (strcmp(nodename, name) == 0)
			return phandle;
	}

	for (child = OF_child(phandle); child; child = OF_peer(child)) {
		if ((node = oskit_linux_ofw_findnode_child(nodename, child)))
			return node;
	}

	return 0;
}

/*
 * Probe for the IDE disks, and partially fill in the details so that
 * actual probe routines will not hang up or do something stupid.
 *
 * This implementation assumes a single IDE interface and at most two
 * disks attached to it. If necessary, the unit number (hwif->index) can
 * be used to locate the proper IDE controller.
 *
 * Also note that the OFW should be used to find the controller and drive
 * information, but since the linux drivers appear to figure things out
 * okay by themselves, I am not going to bother. Tha same cannot be said
 * for the network controller, which requires talking to the OFW to find
 * its configuration.
 */
#include "ide.h"

int
oskit_linux_ofw_probe_ide(ide_hwif_t *hwif)
{
	int		unit;
	int		idenode, child;

	/*
	 * Shut down probe if not the first and only IDE interface.
	 */
	if (hwif->index) {
		hwif->present = 0;
		hwif->noprobe = 1;
		return 0;
	}

	/*
	 * Find the IDE node, if it exists.
	 */
	idenode = oskit_linux_ofw_findnode("ide");
	if (!idenode)
		return 0;

	DPRINTF("idenode = 0x%x\n", idenode);

	/*
	 * Initialize disks to not present and noprobe so that the
	 * the ide probe routine does not hang up.
	 */
	for (unit = 0; unit < MAX_DRIVES; unit++) {
		ide_drive_t *drive = &hwif->drives[unit];

		drive->present = 0;
		drive->noprobe = 1;
	}

	/*
	 * Look for units on the IDE interfaces. Max two.
	 */
	for (unit = 0, child = OF_child(idenode);
	     unit < 2 && child; unit++, child = OF_peer(child)) {
		ide_drive_t *drive = &hwif->drives[unit];

		DPRINTF("idedisk:%d = 0x%x\n", unit, child);

		/*
		 * Okay, let it be probed since it is known to exist.
		 */
		drive->noprobe = 0;
	}

	return 0;
}

/*
 * Probe for the ethernet device. All kinds of information can be gleaned
 * from the OFW, but the cs89x0 driver is quite simple. So, we get the most
 * critical information (mac address, irq) and hardwire the
 * rest. As needed, this can be made more sophisticated, although the linux
 * drivers are not well set up for this.
 *
 * The driver should also pass in a string to compare against so that
 * the actual device is matched to the driver. Again, we can do this
 * later if we ever see a DNARD with a different ethernet card (or more
 * than one card).
 */
#include <linux/if_ether.h>

int
oskit_linux_ofw_probe_ethernet(int *ioaddr, int *irq, unsigned char *mac)
{
	int	node, len, i, region;
	char	*buf, *msg;
	int	*bp;
	char	enaddr[ETH_ALEN];

	/*
	 * Find the node, if it exists.
	 */
	node = oskit_linux_ofw_findnode("ethernet");
	if (!node)
		return -1;

	DPRINTF("node = 0x%x\n", node);

	/*
	 * Get the region information.
	 */
	if ((len = OF_getproplen(node, "reg")) < 0) {
		msg = "Bad region length";
		goto bad;
	}
	DPRINTF("regions len = %d\n", len);

	if ((buf = alloca(len)) == NULL) {
		msg = "Could not alloca regions buffer";
		goto bad;
	}

	if (OF_getprop(node, "reg", buf, len) != len) {
		msg = "Could not get regions information";
		goto bad;
	}

	region = 0;
	for (i = 0, bp = (int *) buf; i < (len / 12); i++, bp += 3) {
		bp[0] = OF_decode_int((unsigned char *) (&bp[0]));
		bp[1] = OF_decode_int((unsigned char *) (&bp[1]));

		DPRINTF("%x %x\n", bp[0], bp[1]);

		if (bp[0] & 1)
			region = bp[1];
	}
	DPRINTF("region = (0x%x)%d\n", region, region);

	/*
	 * If the caller supplied a region, see if it matches. If it
	 * does not match, than this is not the device we think it is.
	 */
	if (*ioaddr && *ioaddr != region)
		return -1;

	*ioaddr = region;

	/*
	 * Get the interrupt information.
	 */
	if ((len = OF_getproplen(node, "interrupts")) < 0) {
		msg = "Bad interrupt length";
		goto bad;
	}
	DPRINTF("interrupts len = %d\n", len);

	if ((buf = alloca(len)) == NULL) {
		msg = "Could not alloca interrupts buffer";
		goto bad;
	}

	if (OF_getprop(node, "interrupts", buf, len) != len) {
		msg = "Could not get interrupts information";
		goto bad;
	}

	*irq = 0;
	for (i = 0, bp = (int *) buf; i < (len / 8); i++, bp += 2) {
		bp[0] = OF_decode_int((unsigned char *) (&bp[0]));
		bp[1] = OF_decode_int((unsigned char *) (&bp[1]));

		DPRINTF("%x %x\n", bp[0], bp[1]);
		
		*irq = bp[0];
	}
	DPRINTF("irq = %d\n", *irq);

	/*
	 * Get the mac address.
	 */
	if (OF_getprop(node, "mac-address", enaddr, sizeof(enaddr)) < 0) {
		msg = "unable to get Ethernet address";
		goto bad;
	}
	DPRINTF("mac: %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x\n",
		enaddr[0], enaddr[1], enaddr[2],
		enaddr[3], enaddr[4], enaddr[5]);

	memcpy(mac, enaddr, sizeof(enaddr));

	return 0;

    bad:
	panic("oskit_linux_ofw_probe_ethernet: %s\n", msg);
	return -1;
}
#endif
