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
 * This file contains the prototype definitions for the DPF analog
 * to the Simple Packet Filter NetIO consumer/producer. 
 */
#ifndef _OSKIT_IO_DPF_H_
#define _OSKIT_IO_DPF_H_

#include <oskit/io/netio.h>
#include <sys/types.h>

/*
 * This is an intermediate netio that only forwards a packet if it
 * matches the predefined filters. The packet will be forwarded to 
 * the out_netio specified here. 
 * 
 * You must call 
 * 
 *   oskit_s32_t
 *   oskit_netio_dpf_insert(void *filter, oskit_s32_t filter_sz);
 *
 * to insert one or more predicates into the filter. See oskit/dpf.h
 * and examples/x86/more/dpf_com for more information.
 */

/*
 * Arguments:
 *          - <in> fid: pointer to an integer that will be set
 *            to the Filter ID that matches packets. 
 *          - <in> out_netio: matching packets will be forwarded
 *            to this netio. 
 *          - <out> push_netio: pass this netio to the device that
 *            will push packets to this dpf object (for example, 
 *            an ethernet netio). 
 * Return:
 *          - 0 if successful, an OSKIT error if not successful. 
 */

oskit_s32_t
oskit_netio_dpf_create(oskit_s32_t *fid, 
		       oskit_netio_t *out_netio, 
		       oskit_netio_t **push_netio);

#endif /* _OSKIT_IO_DPF_H_ */
