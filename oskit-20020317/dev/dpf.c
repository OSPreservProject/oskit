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
 * This is the DPF analog to the Simple Packet Filter interface. For
 * an example, see oskit/examples/x86/more/dpf_com.c.
 */

#ifndef OSKIT
#define OSKIT
#define DEFINED_OSKIT_IN_DPF
#endif

#include <oskit/dev/dev.h>
#include <oskit/dev/net.h>
#include <oskit/io/netio.h>
#include <oskit/c/string.h>
#include <oskit/c/stdlib.h>

#include <oskit/dpf.h>     /* for oskit_dpf_iptr() */

#include <stdio.h>

struct dpfdata {
	oskit_netio_t *netio;
	oskit_s32_t *ppid;
};

/*
 * This is the function that does the filtering. The param
 * is the dpfdata created when the dpf netio was created.
 */
static oskit_error_t
filter_func(void *param, oskit_bufio_t *b, oskit_size_t pkt_size)
{
	struct dpfdata *data = (struct dpfdata *)param;
	char *pkt;
	unsigned char *upkt;
	int err;

	err = oskit_bufio_map(b, (void **)&pkt, 0, pkt_size);
	upkt = (unsigned char *)pkt;
	osenv_assert(!err);
#if 0
	printf("===============================\n\
                %x-%x-%x-%x-%x-%x\n\
                %x-%x-%x-%x-%x-%x\n\
                %x, %x,\n\
                       \n\
                %d, %d,\n\
                %x, %x\n",\
	       upkt[0], upkt[1], upkt[2], upkt[3], upkt[4], upkt[5],
	       upkt[6], upkt[7], upkt[8], upkt[9], upkt[10], upkt[11],
	       upkt[12], upkt[13],
	       upkt[14], upkt[15],
	       upkt[16], upkt[17]);
#endif
	err = oskit_dpf_iptr(pkt, pkt_size);
	*data->ppid = err;
	if (err != 0) {
	       /* Matches, call the netio's push routine.
		* Pid set above after call to oskit_dpf_iptr().
		*/
		oskit_netio_push(data->netio, b, pkt_size);
	}
	/* if doesn't match, just return */

	err = oskit_bufio_unmap(b, (void *)pkt, 0, pkt_size);
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
	osenv_mem_free(param, 0, sizeof(struct dpfdata));
}




/*
 * This function creates a DPF netio. When a matching packet comes in,
 * we'll set *pid to the pid matched by the filter and call out_netio.
 */
int
oskit_netio_dpf_create(int *ppid,
		       oskit_netio_t *out_netio,
		       oskit_netio_t **push_netio)
{
	struct dpfdata *data;
	oskit_netio_t *pnetio;

	data = osenv_mem_alloc(sizeof(*data), 0, 0);
	if (!data)
		return OSKIT_ENOMEM;

	data->netio = out_netio;
	data->ppid = ppid;

	/*
	 * Create a netio that calls filter_func when it receives
	 * a packet with 'data' as the parameter.
	 */
	pnetio = oskit_netio_create_cleanup(filter_func, data, cleanup);
	if (!pnetio)
		return OSKIT_E_UNEXPECTED;	/* XXX */

	*push_netio = pnetio;
	return 0;
}

#undef printf

#ifdef DEFINED_OSKIT_IN_DPF
#undef OSKIT
#undef DEFINED_OSKIT_IN_DPF
#endif
