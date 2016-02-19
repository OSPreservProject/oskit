/*
 * Copyright (c) 1997-1998, 2001 University of Utah and the Flux Group.
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

#ifndef _OSKIT_NET_BOOTP_H_
#define _OSKIT_NET_BOOTP_H_

#include <oskit/types.h>		/* oskit_ */
#include <oskit/net/ether.h>	/* ETHER_ADDR_SIZE */
#include <oskit/dev/ethernet.h>	/* oskit_etherdev_t */
#include <netinet/in.h>		/* in_addr */

/*
 * structure containing the results of a bootp request
 * the dynamic fields (myname, servername, etc.) are allocated by
 * bootp and can be free via bootp_free
 *
 * flags is an ored combination of the fields below and desribes
 * which fields are valid.
 */
#define BOOTP_NET_IP				0x0001
#define BOOTP_NET_NETMASK			0x0002
#define BOOTP_NET_GATEWAY			0x0004
#define BOOTP_NET_SERVER			0x0008
#define BOOTP_NET_DNS_SERVER			0x0010
#define BOOTP_NET_TIME_SERVER			0x0020
#define BOOTP_NET_LOG_SERVER			0x0040
#define BOOTP_NET_LPR_SERVER			0x0080
#define BOOTP_NET_TIME_OFFSET			0x0100
#define BOOTP_NET_HOSTNAME			0x0200
#define BOOTP_NET_SERVER_NAME			0x0400
#define BOOTP_NET_BOOTFILE_NAME			0x0800
#define BOOTP_NET_SERVER_ADDR			0x1000
#define BOOTP_NET_DOMAINNAME			0x2000

/*
 * this structure is used for entries that can return more than one entity
 */
struct  bootp_addr_array {
	struct in_addr *addr;
	int		len;	/* this is in struct in_addr units! */
};

struct bootp_net_info {
	oskit_u32_t     flags;		/* which fields are valid */
	struct in_addr ip;		/* client IP address */
	struct in_addr netmask;		/* subnet mask */
	struct in_addr server;		/* server that replied  */
	struct bootp_addr_array gateway;	/* gateways */
	struct bootp_addr_array dns_server;	/* DNS server address  */
	struct bootp_addr_array time_server;	/* time server address  */
	struct bootp_addr_array log_server;	/* log server address  */
	struct bootp_addr_array lpr_server;	/* LPR server address  */
	/* 
	 * Specifies the time offset of the local subnet in seconds
	 * from Coordinated Universal Time (UTC); signed 32-bit integer
	 * Positive numbers mean west of the Prime Meridian
	 */
	oskit_s32_t     time_offset;	
	char *hostname;			/* client hostname */
	char *server_name;		/* name of replying server */
	char *bootfile_name;		/* boot file name */
	char *domainname;		/* domain name */
	/* server hardware address */
	unsigned char server_hwaddr[ETHER_ADDR_SIZE];	
};

#define BOOTP_MAX_RETRIES	10	/* packets tried */
#define BOOTP_TIMEOUT		200	/* in milliseconds */

/* This returns the netmask based on the network class (from MSB of address) */
struct in_addr bootp_default_netmask(struct in_addr addr);


/* 
 * Fill in bootp_net_info structure with info for the oskit_etherdev_t device,
 * send `retry' number of packets, wait `bootp_timeout_ms' milliseconds for
 * a reply
 */
int bootp_gen(oskit_etherdev_t *trydev, struct bootp_net_info *info,
	int retry, int bootp_timeout_ms);

/* 
 * Same as bootp_gen with retry == BOOTP_MAX_RETRIES 
 * and bootp_timeout == BOOTP_TIMEOUT 
 */
int bootp(oskit_etherdev_t *dev, struct bootp_net_info *info);

/* Print out info from the bootp_net_info structure */
void bootp_dump(struct bootp_net_info *info);

/* Free bootp_net_info structure */
void bootp_free(struct bootp_net_info *info);

#endif /* _OSKIT_NET_BOOTP_H_ */
