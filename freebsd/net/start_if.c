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
 * This code takes a oskit_etherdev_t and prepares a structure
 * which can be used to up that interface via ifconfig.
 */
#include <oskit/dev/error.h>
#include <oskit/dev/dev.h>
#include <oskit/dev/ethernet.h>
#include <oskit/dev/net.h>
#include <oskit/net/freebsd.h>
#include "bsdnet_net_io.h"
#include "osenv.h"
#include <string.h>

/*
 * open dev and return net_ether_if in *out_eif
 */
oskit_error_t
oskit_freebsd_net_open_ether_if(oskit_etherdev_t *dev,
	oskit_freebsd_net_ether_if_t **out_eif)
{
	oskit_freebsd_net_ether_if_t *eif;
	int 	i, err = 0;

	/* open one ethernet card */
        eif = (oskit_freebsd_net_ether_if_t *)osenv_mem_alloc(sizeof *eif, 0, 0);
        memset(eif, 0, sizeof *eif);

        eif->dev = dev;
        eif->recv_nio = oskit_netio_create(bsdnet_net_receive, eif);
        if (eif->recv_nio == NULL) {
                osenv_log(OSENV_LOG_ERR, 
			"oskit_netio_create failed in "__FUNCTION__);
		return OSKIT_ENOMEM;
	}

        oskit_etherdev_getaddr(dev, eif->haddr);
        err = oskit_etherdev_getinfo(dev, &eif->info);

        if (err) {
                osenv_log(OSENV_LOG_ERR, "Error getting info from ethercard");
		return err;
	}

        /* Open this adaptor, exchanging net_io devices */
        err = oskit_etherdev_open(dev, 0, eif->recv_nio, &eif->send_nio);
        if (err) {
                osenv_log(OSENV_LOG_ERR, 
			"Error opening ethercard `%s'", eif->info.name);
		return err;
	}

        /* Show information about this adaptor */
        osenv_log(OSENV_LOG_INFO, "  %-16s%-40s ", eif->info.name,
                eif->info.description ? eif->info.description : "");
        for (i = 0; i < 5; i++)
                osenv_log(OSENV_LOG_INFO, "%02x:", eif->haddr[i]);
        osenv_log(OSENV_LOG_INFO, "%02x\n", eif->haddr[5]);

	*out_eif = eif;
	return 0;
}

/*
 * close the COM instances belonging to an interface
 */
void
oskit_freebsd_net_close_ether_if(oskit_freebsd_net_ether_if_t * eif)
{
        /* close etherdev and release net_io devices */
	oskit_netio_release(eif->send_nio);
	oskit_netio_release(eif->recv_nio);
	if (eif->dev)
		oskit_etherdev_release(eif->dev);
	osenv_mem_free(eif, 0, sizeof *eif);
}

/*
 * prepare a net_ether_if struct with the recv_nio device
 * this struct can be used by the user, who'll have to fill in 
 * the remaining parts
 */
oskit_error_t
oskit_freebsd_net_prepare_ether_if(oskit_freebsd_net_ether_if_t **out_eif)
{
	oskit_freebsd_net_ether_if_t *eif;

	/* open one ethernet card */
        eif = (oskit_freebsd_net_ether_if_t *)osenv_mem_alloc(sizeof *eif, 0, 0);
        memset(eif, 0, sizeof *eif);

	eif->recv_nio = oskit_netio_create(bsdnet_net_receive, eif);
        if (eif->recv_nio == NULL) {
                osenv_log(OSENV_LOG_ERR, 
			"oskit_netio_create failed in "__FUNCTION__);
		return OSKIT_ENOMEM;
	}
	*out_eif = eif;
	return 0;
}

