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

#include <oskit/types.h>
#include <oskit/dpf.h>
#include "../src/dpf/old-dpf.h"
#include "../src/dpf/dpf.h"
#include "../src/dpf/dpf-internal.h"
#include "../src/dpf/dpf_lib.h"

/* 
 * For some reason, mk_udp() has no prototype
 * in the original src code for some of these 
 * functions. We'll just use externs here.
 */
extern void *mk_udp(int *sz, int src_port, int dst_port);
extern void *mk_pathfinder(int *sz, int src_port, int dst_port);
extern void *mk_short_path(int *sz, int src_port);
extern void *mk_short_path2(int *sz, int dst_port);

/* set equal to dpf_iptr */
oskit_s32_t (*oskit_dpf_iptr)(unsigned char *msg, unsigned nbytes);

void oskit_dpf_init(void) 
{
	return;
}

void *oskit_dpf_mk_udp(oskit_s32_t *sz, 
		       oskit_s32_t src_port, 
		       oskit_s32_t dst_port) 
{
	return mk_udp(sz, src_port, dst_port);
}

void *oskit_dpf_mk_udpu(oskit_s32_t *sz, 
			oskit_u16_t src_port, 
			oskit_u8_t *src_ip) 
{
	return mk_udpu(sz, src_port, src_ip);
}

void *oskit_dpf_mk_udpc(oskit_s32_t *sz,
	                oskit_u16_t src_port, 
			oskit_u8_t *src_ip, 
			oskit_u16_t dst_port, 
			oskit_u8_t *dst_ip)
{
	return mk_udpc(sz, src_port, src_ip, dst_port, dst_ip);
}

void *oskit_dpf_mk_rarp(oskit_s32_t *sz, void *ip_addr) 
{
	return (void *)mk_rarp(sz, ip_addr);
}

void *oskit_dpf_mk_arp_req(oskit_s32_t *sz, oskit_u8_t *ipaddr)
{
	return (void *)mk_arp(sz, ipaddr);
}

void *oskit_dpf_mk_arp_rep(oskit_s32_t *sz, oskit_u8_t *ipaddr)
{
	/* 
	 * There is no corresponding arp_req, so 
	 * we just change the bit at this level to
	 * avoid having to change the dpf code.
	 */

	struct dpf_frozen_ir *filter = 
		(struct dpf_frozen_ir *)mk_arp(sz, ipaddr);
	filter[6].imm = 0x200;    /* reply, not request */
	return (void *)filter;
}

void oskit_dpf_mk_udpc_string(oskit_s8_t *new, 
			      oskit_u16_t src_port, 
			      oskit_u8_t *src_ip, 
			      oskit_u16_t dst_port, 
			      oskit_u8_t *dst_ip)
{
	mk_udpc_string(new, src_port, src_ip, dst_port, dst_ip);
}

void oskit_dpf_mk_arp_req_string(oskit_s8_t *new, oskit_u8_t *ethaddr)
{
	mk_arp_string(new, ethaddr);
}

static unsigned char arp_rep_string[] = {
	/* Ethernet Layer */
	0, 0, 0, 0, 0, 0,	/* ether destination */
	0, 0, 0, 0, 0, 0,	/* ether source */
	0x08, 0x06,		/* packet type ie ARP */

	/* ARP Layer */
	0, 1,			/* Hardware Protocol */
	0x08, 0x00,		/* Target Protocol */
	6, 4,			/* Har Len, Tar Len */
	0, 2,			/* Operation (response to query) */
	0, 0, 0, 0, 0, 0,	/* Sender Hardware (ether) */
	0, 0, 0, 0,		/* Sender Target (IP) */
	0, 0, 0, 0, 0, 0,	/* Response Hardware */
	0, 0, 0, 0		/* Response IP */
};

void 
mk_arp_rep_string(char *new, unsigned char *ipaddr)
{
	int     sz = (sizeof arp_rep_string);
	memcpy(new, arp_rep_string, sz);
	memcpy(new + 26, ipaddr, 4);
}

void oskit_dpf_mk_arp_rep_string(oskit_s8_t *new, oskit_u8_t *ethaddr)
{
	mk_arp_rep_string(new, ethaddr);
	
}

void *oskit_dpf_mk_pathfinder(oskit_s32_t *sz, 
			      oskit_s32_t src_port, 
			      oskit_s32_t dst_port) 
{
	return mk_pathfinder(sz, src_port, dst_port);
}

void oskit_dpf_mk_rarp_string(oskit_s8_t *new, oskit_u8_t *ethaddr)
{
	mk_rarp_string(new, ethaddr);
}

void *oskit_dpf_mk_short_path(oskit_s32_t *sz, oskit_s32_t src_port) 
{
	return mk_short_path(sz, src_port);
}

void *oskit_dpf_mk_short_path2(oskit_s32_t *sz, oskit_s32_t dst_port)
{
	return mk_short_path2(sz, dst_port);
}

/* Filter insertion/deletion. */
oskit_s32_t oskit_dpf_insert(void *p, oskit_s32_t sz)
{
	oskit_s32_t retval;

	retval = dpf_insert(p, sz);
	oskit_dpf_iptr = dpf_iptr;

	return retval;
} 

oskit_s32_t oskit_dpf_delete(oskit_u32_t pid)
{
	return dpf_delete(pid);
}

void oskit_dpf_verbose(oskit_s32_t v)
{
	dpf_verbose(v);
}


void oskit_dpf_output(void)
{
	dpf_output();
}
