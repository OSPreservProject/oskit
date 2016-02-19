/*
 * Copyright (c) 1994-1997 The University of Utah and the Flux Group.
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
 * This Packet Dispatcher supports two functions: creation
 * and packet description insertion. The format of the packet
 * description is dependent upon the type of filter machinery
 * specified in the creation call. The filters supported 
 * currently include:
 * 
 *        - DPF (see <oskit/dpf.h> for filter template functions)
 */

#define OSKIT_PD_FILTER_DPF_INTERP   1

#include <oskit/io/netio.h>
/*#include "../dev/pd.h" */

#include <com/sfs_hashtab.h>

typedef struct opdt {
	hashtab_t ht;
	oskit_netio_t *default_netio;
	oskit_netio_t *pf_netio;         /* packet filter netio */
	oskit_u32_t ft;                  /* filter type */
} oskit_pd_t;


/*
 * Create a packet dispatcher. The user can, for example, 
 * pass the value of *push_netio to oskit_etherdev_open(). 
 * 
 * Arguments:
 *        - <out> pkt_dispatcher: pointer to the pkt_dispatcher
 *          just created. 
 *        - <out> push_netio: the netio from which the packet
 *          dispatcher will receive packets (pass to 
 *          oskit_etherdev_open(), for example).
 *        - <in> default_outgoing: the netio to which packets
 *          not matching any specified filter are pushed by the
 *          dispatcher. 
 * Returns: 
 *        - 0 if successful,  OSKit error otherwise.
 */
oskit_s32_t
oskit_packet_dispatcher_create(oskit_pd_t **pd,
			       oskit_u32_t filter_type,
	                       oskit_netio_t **push_netio,
			       oskit_netio_t *default_outgoing);

/*
 * Arguments:
 *        - <in> pkt_dispatcher: pointer to the packet_dispatcher
 *          created in the create() call above. 
 *        - <out> pdid: ID of the resulting filter. The user should
 *          use this value to identify the packet descr when calling 
 *          oskit_packet_dispatcher_delete(). The value is undefined
 *          if insertion wasn't successful (see return value). 
 *        - <in> out_netio: pointer to the netio to which packets
 *          matching pdescr will be pushed. 
 *        - <in> pdescr: a description of the packet. The format
 *          of the packet description is dependent upon the
 *          type of filter machinery specified in the creation call. 
 *          See above for supported filters. 
 * Returns: 
 *        - 0 if successful,  OSKit error otherwise.
 */
oskit_s32_t
oskit_packet_dispatcher_register(oskit_pd_t *pkt_dispatcher,
				 oskit_s32_t *pdid, 
				 oskit_netio_t *out_netio,
				 void *pdescr,
				 oskit_s32_t sz);

/* 
 * Arguments:
 *        - <in> pkt_dispatcher: pointer to the packet_dispatcher
 *          created in the create() call above. 
 *        - <in> fid: Filter ID previously returned from a call
 *          to oskit_packet_dispatcher_insert() above. 
 * Returns: 
 *        - 0 is successful, OSKit error otherwise. 
 */
oskit_s32_t
oskit_packet_dispatcher_delete(oskit_pd_t *pkt_dispatcher,
			       oskit_s32_t fid);





