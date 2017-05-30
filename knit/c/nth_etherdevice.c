/*
 * Copyright (c) 2000 University of Utah and the Flux Group.
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

#include <oskit/dev/ethernet.h>
#include <oskit/net/ether.h>
#include <oskit/dev/dev.h>
#include <stdlib.h>

extern int n;

oskit_etherdev_t *etherdev = 0;
unsigned char ethaddr[ETHER_ADDR_SIZE] = {0};

oskit_error_t
init(void)
{
	oskit_etherdev_t **etherdevs;
	oskit_size_t ndev;
	int i;

	/*
	 * Find all the Ethernet device nodes, BUT ONLY USE THE nth ONE.
	 */
	ndev = osenv_device_lookup(&oskit_etherdev_iid, (void***)&etherdevs);

        if (ndev > n) {
                etherdev = etherdevs[n];
                oskit_etherdev_getaddr(etherdev, ethaddr);

                for (i = 0; i < ndev; i++) {
                        if (i != n)
                                oskit_etherdev_release(etherdevs[i]);
                }
                free(etherdevs);
                return 0;
        } else {
                for (i = 0; i < ndev; i++) {
                        oskit_etherdev_release(etherdevs[i]);
                }
                free(etherdevs);
                return OSKIT_ENODEV;
        }
}


oskit_error_t
fini(void)
{
        if (etherdev) {
                oskit_etherdev_release(etherdev);
                etherdev = 0;
        }
        return 0;
}
