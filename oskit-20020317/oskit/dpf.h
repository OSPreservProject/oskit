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

#ifndef _OSKIT_DPF_H_
#define _OSKIT_DPF_H_

#include <oskit/types.h>

/* Errors */
enum { OSKIT_DPF_TOOMANYFILTERS = -2, 
       OSKIT_DPF_TOOMANYELEMS = -3, 
       OSKIT_DPF_BOGUSOP = -4, 
       OSKIT_DPF_BOGUSID = -5, 
       OSKIT_DPF_OVERLAP = -6, 
       OSKIT_DPF_NILFILTER = -7};

/* Maximum number of elements in a DPF filter. */
#define OSKIT_DPF_MAXELEM 256

/* Must call this before any other dpf functions. */
void oskit_dpf_init(void);

/* Make some filters with given parameters. */
void *oskit_dpf_mk_udp(oskit_s32_t *sz, oskit_s32_t src_port, oskit_s32_t dst_port);
void *oskit_dpf_mk_udpu(oskit_s32_t *sz, oskit_u16_t src_port, oskit_u8_t *src_ip);
void *oskit_dpf_mk_udpc(oskit_s32_t *sz, 
			oskit_u16_t src_port, oskit_u8_t *src_ip, 
			oskit_u16_t dst_port, oskit_u8_t *dst_ip);
void *oskit_dpf_mk_pathfinder(oskit_s32_t *sz, oskit_s32_t src_port, oskit_s32_t dst_port);
void *oskit_dpf_mk_short_path(oskit_s32_t *sz, oskit_s32_t src_port);
void *oskit_dpf_mk_short_path2(oskit_s32_t *sz, oskit_s32_t dst_port);
void *oskit_dpf_mk_rarp(oskit_s32_t *sz, void *ip_addr);
void *oskit_dpf_mk_arp_req(oskit_s32_t *sz, oskit_u8_t *ipaddr);
void *oskit_dpf_mk_arp_rep(oskit_s32_t *sz, oskit_u8_t *ipaddr);

/* 
 * These are helpful for making fake packets for debugging. The 'new' arg
 * is overwritten with a default template then customized according to
 * the given arguments. For example, mk_rarp_string() copies the appropriate
 * values for a RARP packet into 'new' and then customizes 'new' by adding
 * in the given ethernet address. 
 * 
 * NOTE: These are dangerous currently because we don't give the caller
 * any idea of how long their 'new' buffer should be. Should add some
 * API for that. -LKW
 */
void oskit_dpf_mk_rarp_string(oskit_s8_t *new, oskit_u8_t *ethaddr);
void oskit_dpf_mk_udpc_string(oskit_s8_t *new, 
			      oskit_u16_t src_port, oskit_u8_t *src_ip, 
			      oskit_u16_t dst_port, oskit_u8_t *dst_ip);
void oskit_dpf_mk_arp_req_string(oskit_s8_t *new, oskit_u8_t *ipaddr);
void oskit_dpf_mk_arp_rep_string(oskit_s8_t *new, oskit_u8_t *ipaddr);

/* Filter insertion/deletion. */
oskit_s32_t oskit_dpf_insert(void *p, oskit_s32_t sz); 
oskit_s32_t oskit_dpf_delete(unsigned pid);


/* Debugging support */
void oskit_dpf_verbose(oskit_s32_t v);
void oskit_dpf_output(void);

/* 
 * Called to classify a packet. Returns either the non-zero 
 * previously returned by a call to oskit_dpf_insert() of the
 * predicate(s) that matched or 0. 
 */
oskit_s32_t (*oskit_dpf_iptr)(oskit_u8_t *msg, oskit_u32_t nbytes);

#endif /* _OSKIT_DPF_H */

