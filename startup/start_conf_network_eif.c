/*
 * Copyright (c) 1997-1999, 2001 University of Utah and the Flux Group.
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
 * start_default_network.c
 *
 * Start a network interfaces using the given information.
 */
#include <oskit/startup.h>
#include <oskit/dev/error.h>
#include <oskit/dev/ethernet.h>
#include <oskit/dev/freebsd.h>
#include <oskit/dev/net.h>
#include <oskit/dev/linux.h>
#include <oskit/net/bootp.h>
#include <oskit/misc/sysconf.h>
#include <oskit/com/services.h>

#include <oskit/c/arpa/inet.h>
#include <oskit/c/assert.h>
#include <oskit/c/errno.h>
#include <oskit/c/fcntl.h>
#include <oskit/c/fd.h>
#include <oskit/c/stdarg.h>
#include <oskit/c/stdio.h>
#include <oskit/c/stdlib.h>
#include <oskit/c/string.h>
#include <oskit/c/unistd.h>

#define VERBOSITY 0

/*
 * These are global so release_network() can see them.
 */
static oskit_etherdev_t **edev;
static struct oskit_freebsd_net_ether_if **eif;
static int eif_count = -1;

static int init_network(void);
static void release_network(void *);

/*
 * Open and configure the specified interface with the provided info.
 */
int
start_conf_network_eifstart(int ifn, const char *eif_addr, const char *eif_mask)
{
	const char *eif_name;
	int nif, err;

	if (eif_addr == 0 || eif_mask == 0)
		return OSKIT_EINVAL;

	nif = init_network();
	if (ifn >= nif)
		return OSKIT_EINVAL;

	eif_name = start_conf_network_eifname(ifn);
	if (eif_name == 0)
		return OSKIT_EINVAL;

#if VERBOSITY > 1
	printf("Configuring ether dev %d as %s\n", ifn, eif_name);
#endif
		
	/*
	 * Open the underlying ether device and configure it.
	 */
#if VERBOSITY > 2
	printf("  open %s\n", eif_name);
#endif
	err = start_conf_network_open_eif(edev[ifn], &eif[ifn]);
	if (err == 0) {
#if VERBOSITY > 2
		printf("  ifconfig %s as %s (mask %s)\n",
		       eif_name, eif_addr, eif_mask);
#endif
		err = start_conf_network_eifconfig(eif[ifn], eif_name,
						   eif_addr, eif_mask);
	}

#if VERBOSITY > 2
	if (err != 0)
		printf("  start_conf_network_single(%d) FAILED (err=%x)\n",
		       ifn, err);
#endif

	return err;
}

/*
 * "Down" and close the indicated interface.
 * The reverse of eifstart.
 */
int
start_conf_network_eifstop(int ifn)
{
	int err;

	if (ifn < 0 || ifn >= eif_count)
		return OSKIT_EINVAL;

#if VERBOSITY > 1
	printf("Shutting down and closing ether dev %d\n", ifn);
#endif
	err = start_conf_network_eifdown(eif[ifn]);
	if (err != 0) {
#if VERBOSITY > 2
		printf("  eifdown of %d failed with err=%x\n", ifn, err);
#endif
		return err;
	}

	start_conf_network_close_eif(eif[ifn]);
	eif[ifn] = NULL;
	return err;
}

/*
 * One time setup.
 */
static int
init_network(void)
{
	if (eif_count >= 0)
		return eif_count;

	start_net_devices();

        eif_count = osenv_device_lookup(&oskit_etherdev_iid, (void***)&edev);
	assert(eif_count >= 0);
	if (eif_count == 0)
		return 0;

	eif = osenv_mem_alloc(sizeof(*eif) * eif_count, 0, 0);
	assert(eif != 0);
	memset(eif, 0, sizeof(*eif) * eif_count);

	startup_atexit(release_network, NULL);

	return eif_count;
}

/*
 * Exit hook to shut down the network device. Going to sleep 1 second
 * to allow the network to settle. 
 */
static void
release_network(void *arg)
{
	int i;
#if VERBOSITY > 0
	printf("Shutting down network ... ");
#endif
	sleep(1);
	
	/* close etherdev and release net_io devices */
	for (i = 0; i < eif_count; i++) {
		if (eif[i] != NULL)
			start_conf_network_close_eif(eif[i]);
	}
	osenv_mem_free(eif, 0, sizeof(*eif) * eif_count);
	free(edev);

#if VERBOSITY > 0
	printf("Done!\n");
#endif
}
