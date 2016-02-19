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

/* Modified from FreeBSD 2.1 sys/i386/boot/netboot */
/**************************************************************************
NETBOOT -  BOOTP/TFTP Bootstrap Program

Author: Martin Renters
  Date: Dec/93

**************************************************************************/

#ifndef __BOOTP_H_INCLUDED__
#define __BOOTP_H_INCLUDED__

#include <oskit/net/bootp.h>
#include "udp.h"

struct bootp_t {
	struct iphdr ip;
	struct udphdr udp;
	char bp_op;
	char bp_htype;
	char bp_hlen;
	char bp_hops;
	unsigned long bp_xid;
	unsigned short bp_secs;
	unsigned short unused;
	char bp_ciaddr[4];
	char bp_yiaddr[4];
	char bp_siaddr[4];
	char bp_giaddr[4];
	char bp_hwaddr[16];
	char bp_sname[64];
	char bp_file[128];
	char bp_vend[64];
};

/* Ports for BOOTP. */
#define BOOTP_SERVER	67
#define BOOTP_CLIENT	68

/* Values for bp_op. */
#define BOOTP_REQUEST	1
#define BOOTP_REPLY	2

/* Stuff for RFC1048. */
#define TAG_LEN(p)		(*((p)+1))
#define RFC1048_COOKIE		{ 99, 130, 83, 99 }
#define RFC1048_PAD		0
#define RFC1048_NETMASK		1
#define RFC1048_TIME_OFFSET	2
#define RFC1048_GATEWAY		3
#define RFC1048_TIME_SERVER	4
#define RFC1048_IEN116_NAME_SERVER	5	/* unsupported */
#define RFC1048_DNS_SERVER	6
#define RFC1048_LOG_SERVER	7
#define RFC1048_COOKIE_SERVER	8		/* unsupported */
#define RFC1048_LPR_SERVER	9
#define RFC1048_IMPRESS_SERVER	10		/* unsupported */
#define RFC1048_RLP_SERVER	11		/* unsupported */
#define RFC1048_HOSTNAME	12
#define RFC1533_BOOTFILESIZE 	13		/* unsupported */
#define RFC1533_MERITDUMPFILE 	14		/* unsupported */
#define RFC1533_DOMAINNAME	15
#define RFC1533_SWAPSERVER	15		/* unsupported */
#define	RFC1533_ROOTPATH	17		/* unsupported */
#define RFC1533_EXTENSIONSPATH	18		/* unsupported */
/*
 * and a whole buttload of stuff we don't support
 */

#define RFC1048_END		255
#define BOOTP_VENDOR_LEN	64

int bootp_try(char *hwaddr, unsigned long srcip, int retry, int bootp_timeout);
void bootp_decode_packet(struct bootp_t *packet, struct bootp_net_info *info);

/* send ethernet packet */
int bootp_eth_transmit(char *src, char *dst, 
	unsigned short type, unsigned short size, void *buf);

#define PRF     "bootp: "

#define BOOTP_TIMER_FREQ	100
int bootp_currticks();

#endif /* __BOOTP_H_INCLUDED__ */
