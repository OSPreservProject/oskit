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
/*
 * This is the Simple Packet Filter interface.
 * It filters the input from a netio object and passes it to annother.
 */

#include <oskit/dev/dev.h>
#include <oskit/dev/net.h>
#include <oskit/io/netio.h>
#include <oskit/c/string.h>
#include <oskit/c/stdlib.h>

#if 0
/*
 * Example use:
 */
	/*
	 * Preconditions:
	 *
	 * filtered_netio is an initialized netio object that will
	 * receive the filtered packets.
	 *
	 * Variables:
	 *
	 * fnetio_out is used to transmit packets
	 * fnetio_in is the netio that receives raw incomming packets
	 *
	 * device_push_netio *could* be used to bypass the outgoing PF
	 */

	/* incoming fanout */
	fnetio_in = oskit_netio_fanout_create();

	/* outgoing fanout */
	fnetio_out = oskit_netio_fanout_create();

	/*
	 * Open the ethernet device, with incomming_netio as         
	 * the receiver; get the device send netio device_push_netio
	 * it initializes device_push_netio
	 */
	err = oskit_etherdev_open(etherdev, FLAGS, fnetio_in,
	        &device_push_netio);
	osenv_assert(!err);

	/* add device netio to outgoing fanout */
	oskit_netio_fanout_add_listener(fnetio_out, device_push_netio);

	/* 
	 * fanout keeps a ref, so we can release ours, if the fanout is
	 * destroyed later down the road, it'll release the ref to the
	 * device
	 */
	oskit_netio_release(device_push_netio);

	/* 
	 * Create a SPF that receives packets from push_netio and
	 * forwards them to filtered_netio if they match the params
	 * it initializes push_netio
	 */
	oskit_netio_spf_create(..., filtered_netio, &push_netio);
   
	/* make sure pf receives incoming packets */
	oskit_netio_fanout_add_listener(fnetio_in, push_netio);     

	/*
	 * We want to get filtered packets that are being transmitted 
	 * as well as received:
	 */
	oskit_netio_fanout_add_listener(fnetio_out, push_netio);   
	 
	/*
	 * Okay, now filtered_netio is receiving all packets that
	 * match the filter, for both incomming and outgoing packets.
	 * device_push_netio is used to transmit packets
	 */

/*
 * End example
 */
#endif


struct spfdata {
	oskit_netio_t *netio;
	char *filter;
	int len;
	int offset;
};

/*
 * This is the function that does the filtering.
 */
static oskit_error_t
filter_func(void *param, oskit_bufio_t *b, oskit_size_t pkt_size)
{
	struct spfdata *data = (struct spfdata *)param;
	char *pkt;
	int err;

	/* if packet isn't big enough... */
	if (data->len + data->offset > pkt_size) {
		return 0;
	}

	/* check to see if the bufio matches the filter */
	err = oskit_bufio_map(b, (void **)&pkt, data->offset, data->len);
	osenv_assert(!err);

	if (!memcmp(data->filter, pkt, data->len)) {
		/* matches, call the netio's push routine */
		oskit_netio_push(data->netio, b, pkt_size);
	}
	/* if doesn't match, just return */

	err = oskit_bufio_unmap(b, (void *)pkt, data->offset, data->len);
	osenv_assert(!err);

	return 0;
}


/*
 * This is called when the netio object created is no longer used.
 * We `cleanup' -- release the storage, since there are
 * no more references to it anyway.
 */
static void
cleanup(void *param)
{
	struct spfdata *s = param;
	osenv_mem_free(s->filter, 0, s->len);
	osenv_mem_free(param, 0, sizeof(struct spfdata));
}


/*
 * This function creates a SPF netio.
 */
int
oskit_netio_spf_create(char *filter, int len, int offset,
		oskit_netio_t *out_netio, oskit_netio_t **push_netio)
{
	struct spfdata *data;
	oskit_netio_t *pnetio;

	data = osenv_mem_alloc(sizeof(*data), 0, 0);
	if (!data)
		return OSKIT_ENOMEM;

	data->netio = out_netio;
	data->filter = osenv_mem_alloc(len, 0, 0);
	if (!data->filter)
		return OSKIT_ENOMEM;

	memcpy(data->filter, filter, data->len = len);
	data->offset = offset;

	pnetio = oskit_netio_create_cleanup(filter_func, data, cleanup);
	if (!pnetio)
		return OSKIT_E_UNEXPECTED;	/* XXX */

	*push_netio = pnetio;
	return 0;
}


