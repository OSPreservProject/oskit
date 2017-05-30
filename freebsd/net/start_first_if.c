/*
 * Copyright (c) 1997-1999 University of Utah and the Flux Group.
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
 * This code finds the first ethernet interface and prepares a structure
 * which can be used to up that interface via ifconfig.
 */
#include <oskit/dev/error.h>
#include <oskit/dev/dev.h>
#include <oskit/dev/ethernet.h>
#include <oskit/net/freebsd.h>
#include "osenv.h"

/*
 * find first ethernet device, open it and prepare net_ether_if struct
 */
oskit_error_t
oskit_freebsd_net_open_first_ether_if(
	struct oskit_freebsd_net_ether_if **out_eif)
{
	oskit_etherdev_t **etherdev;

        /*
         * Find all the Ethernet device nodes.
         */
        int ndev = osenv_device_lookup(&oskit_etherdev_iid, (void***)&etherdev);
        if (ndev <= 0) {
                osenv_log(OSENV_LOG_WARNING, __FUNCTION__
			": no ethernet adaptors found!");
		return OSKIT_ENODEV;
	}
	/* XXX what happens to the references in the other etherdev[] fields */
	return oskit_freebsd_net_open_ether_if(etherdev[0], out_eif);
}
