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

#include <oskit/clientos.h>
#include <oskit/io/netio.h>
#include <oskit/dpf.h>
#include <oskit/pd.h>
#include <malloc.h>
#include <stdio.h> /* for debugging printf()s */

/*#include "pd.h"*/
#include "../com/sfs_hashtab.h"    

#define HASHTAB_SIZE 1024

void
hashremove(hashtab_key_t k, hashtab_datum_t d, void *args) {}

oskit_u32_t
hashfunc(hashtab_key_t key) 
{
	oskit_u32_t h = (oskit_u32_t) key;
	return h % HASHTAB_SIZE;
}

/*
 * This is called when the netio object created is no longer used.
 * We `cleanup' -- release the storage, since there are
 * no more references to it anyway.
 */
static void
cleanup(void *param)
{
	osenv_mem_free(param, 0, sizeof(oskit_pd_t));
}

/* Our keys are really just integers */
oskit_s32_t keycmpfunc(hashtab_key_t key1, hashtab_key_t key2) 
{
	return !(key1 == key2);
}

static oskit_error_t
recv(void *param, oskit_bufio_t *b, oskit_size_t pkt_size)
{
	oskit_pd_t *pd = (oskit_pd_t *)param;
	oskit_netio_t *netio;
	oskit_s32_t fid;
	oskit_s32_t err;
	char *pkt;

	osenv_assert(param);
	osenv_assert(b);

	err = oskit_bufio_map(b, (void **)&pkt, 0, pkt_size);
	if (err) {
		return OSKIT_E_UNEXPECTED;
	}

	switch (pd->ft) {
	case OSKIT_PD_FILTER_DPF_INTERP:
		fid = oskit_dpf_iptr(pkt, pkt_size);
		break;
	default:
		return OSKIT_E_NOTIMPL;
	}

	if (fid != 0) {
		netio = (oskit_netio_t *)hashtab_search(pd->ht, (hashtab_key_t)fid);
		osenv_assert(netio);
		oskit_netio_push(netio, b, pkt_size);
	} else {
		oskit_netio_push(pd->default_netio, b, pkt_size);
	}
	
	return 0;
}

#ifndef NULL
#define NULL 0
#endif // NULL

oskit_s32_t
oskit_packet_dispatcher_create(oskit_pd_t **ppd,
			       oskit_u32_t ft,
	                       oskit_netio_t **push_netio,
			       oskit_netio_t *default_outgoing)
{
	oskit_netio_t *pnetio;
	oskit_pd_t *pd;
	oskit_s32_t err;

	osenv_assert(ppd);
	osenv_assert(push_netio);
	osenv_assert(default_outgoing);

	/* create a new oskit_pd_t struct. */
	if ((*ppd = (oskit_pd_t *)malloc(sizeof *pd)) == NULL) {
		return OSKIT_ENOMEM;
	}
	pd = *ppd; /* for tidiness */

	/* create the PID table */
	if ((pd->ht = hashtab_create(hashfunc, keycmpfunc, HASHTAB_SIZE)) == NULL) {
		err = OSKIT_ENOMEM;
		goto error;
	}		

	pd->default_netio = default_outgoing;
	pd->ft = ft;

	/*
	 * Set up the filter netio.
	 * Probably all types will use this same code.
	 */
	switch (ft) {
	case OSKIT_PD_FILTER_DPF_INTERP:
		pnetio = oskit_netio_create_cleanup(recv, pd, cleanup);
		if (!pnetio) {
			err = OSKIT_E_UNEXPECTED;
			goto error;
		}
		*push_netio = pnetio;
		break;
	default:
		err = OSKIT_E_NOTIMPL;
		goto error;
	}
	
	return 0;

error:
	*ppd = 0;
	free(*ppd);
	return err;
}

oskit_s32_t
oskit_packet_dispatcher_register(oskit_pd_t *pd,
				 oskit_s32_t *pfid, 
				 oskit_netio_t *out_netio,
				 void *pdescr,
				 oskit_s32_t sz)
{
	oskit_s32_t hterr;

	osenv_assert(pd);
	osenv_assert(pfid);
	osenv_assert(out_netio);
	osenv_assert(pdescr);

	switch (pd->ft) {
	case OSKIT_PD_FILTER_DPF_INTERP:
		*pfid = oskit_dpf_insert(pdescr, sz);
		osenv_assert(*pfid);
		break;
	default:
		return OSKIT_E_NOTIMPL;
	}


	hterr = hashtab_insert(pd->ht, (hashtab_key_t) *pfid, 
                               (hashtab_datum_t)out_netio);
	if (hterr != HASHTAB_SUCCESS) {
		oskit_dpf_delete(*pfid);
		*pfid = 0;
		return OSKIT_E_UNEXPECTED;
	}
     
	return 0;
}


oskit_s32_t
oskit_packet_dispatcher_delete(oskit_pd_t *pd,
			       oskit_s32_t fid)
{
	oskit_s32_t hterr;

	osenv_assert(pd);

	switch (pd->ft) {
	case OSKIT_PD_FILTER_DPF_INTERP:
		oskit_dpf_delete(fid);
		break;
	default:
		return OSKIT_E_NOTIMPL;
	}

	hterr = hashtab_remove(pd->ht, (hashtab_key_t) fid, hashremove, 0);
	if (hterr != HASHTAB_SUCCESS) {
		return OSKIT_E_UNEXPECTED;
	}
	
	return 0;
}


