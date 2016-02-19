/*
 * Copyright (c) 1997-2000 University of Utah and the Flux Group.
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
 * Code derived from boot/net/driver.c
 */

#include <oskit/c/stdio.h>
#include <oskit/net/bootp.h>
#include <oskit/c/stdlib.h>
#include <oskit/c/string.h>

/* ToDo: move to a proper home */
#include <boot/net/getline.h>

extern unsigned int required_flags;

oskit_etherdev_t      *ether_dev  = 0;
struct bootp_net_info  bootp_info = {};

/*
 * Report if flag appears on list of missing flags.
 */
static void
check_flag(unsigned int missing, unsigned int flag, char *name)
{
        if (missing & flag) {
                printf("bootp did not supply %s\n", name);
        }
}

/*
 * Try to bootp using device 'dev'.
 */
static oskit_error_t
try_bootp(oskit_etherdev_t *dev)
{
        oskit_error_t rc;
        unsigned int missing;

	memset(&bootp_info, 0, sizeof bootp_info);
        rc = bootp(dev, &bootp_info);
        if (rc) 
                return rc;

        missing = (~bootp_info.flags) & required_flags;
        if (missing) {
                check_flag(missing, BOOTP_NET_IP,       "my IP address");
                check_flag(missing, BOOTP_NET_NETMASK,  "my netmask");
                check_flag(missing, BOOTP_NET_GATEWAY,  "gateway address");
                check_flag(missing, BOOTP_NET_HOSTNAME, "my hostname");
                return OSKIT_E_FAIL;
        }
#if 1
        printf("Got the following information from bootp\n");
        bootp_dump(&bootp_info);
#endif
        return 0;
}

/*
 * Return first device that successfully bootp's
 */
static oskit_etherdev_t*
try_bootps(int ndev, oskit_etherdev_t **devs)
{
        int i;
        for (i = 0; i < ndev; i++) {
                printf("Trying device %d.\n", i);
                if (!try_bootp(devs[i])) {
                        return devs[i];
                }
        }
        return 0;
}

/* 
 * Release all devices in devs.
 */
static void
free_devices(int ndev, oskit_etherdev_t **devs)
{
        int i;
        for (i = 0; i < ndev; i++) {
                oskit_etherdev_release(devs[i]);
        }
        free(devs);
}

oskit_error_t
init(void)
{
	oskit_etherdev_t **alldevs;
	int ndev;

	ndev = osenv_device_lookup(&oskit_etherdev_iid, (void ***)&alldevs);
	if (ndev <= 0) {
		printf("No Ethernet adaptors found!");
                return OSKIT_E_FAIL;
        }

        /*
         * Find a device that successfully bootp's
         */
        printf("Looking for a device that answers bootp requests.\n");
        do {
                ether_dev = try_bootps(ndev, alldevs);
#if 1
                if (ether_dev == 0) {
#define BUFSIZE 128
                        char buf[BUFSIZE];
                        printf("No bootp server found for any interfaces!  Try again? [n] ");
                        getline(buf, BUFSIZE);
                        if (!(buf[0] == 'y' || buf[0] == 'Y')) {
                                free_devices(ndev,alldevs);
                                return OSKIT_E_FAIL;
                        }
                }
#endif
        } while (!ether_dev);

        /*
         * We've now found a device, keep it and discard the others.
         */
        oskit_etherdev_addref(ether_dev);
        free_devices(ndev,alldevs);
        
        return 0;
}

oskit_error_t
fini(void)
{
        if (ether_dev) {
                oskit_etherdev_release(ether_dev);
        }
        bootp_free(&bootp_info);
        return 0;
}
