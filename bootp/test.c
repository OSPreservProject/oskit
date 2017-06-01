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

#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <oskit/dev/dev.h>
#include <oskit/dev/linux.h>
#include <oskit/dev/net.h>
#include <oskit/dev/ethernet.h>
#include <oskit/dev/error.h>

#include "bootp.h"


int
bootp_test()
{
	static struct bootp_net_info INFO;
	int ndev, i, rc = 0 /* gcc warning */;
	oskit_etherdev_t **alldevs;
	int count = 0;

        ndev = osenv_device_lookup(&oskit_etherdev_iid, (void ***)&alldevs);
        if (ndev <= 0)
                panic("no Ethernet adaptors found!");

 retry: for (i = 0; i < ndev; i++) {
		rc = bootp(alldevs[i], &INFO);
		if (!rc) {
			count++;
			bootp_dump(&INFO);
		}
        }

	if (!count) {
        	char buf;
        	printf("No bootp server found for any interfaces!  Try again? [n] ");
		buf = getchar();
		/* XXX: newline here */
        	if (buf == 'y' || buf == 'Y')
                	goto retry;

        	return 1;
	}

	return 0;
}

