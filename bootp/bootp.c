/*
 * Copyright (c) 1997-1998, 2000 University of Utah and the Flux Group.
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


#include <oskit/c/string.h>		/* memcpy */
#include <oskit/c/arpa/inet.h>		/* ntohs */
#include <oskit/c/stdlib.h>		/* mustcalloc */
#include <oskit/c/stdio.h>

#include <oskit/time.h>
#include <oskit/dev/dev.h>
#include <oskit/com/listener.h>
#include <oskit/dev/clock.h>
#include <oskit/dev/timer.h>

#include "udp.h"
#include "bootp.h"

/* this flag is set upon receive of a valid bootp packet */
extern volatile int bootp_reply;


/*
 * Get my IP address and load information
 */
int
bootp_try(char *hwaddr, unsigned long srcip, int retry, int bootp_timeout)
{
	struct bootp_t bp;
	static const char rfc1048_cookie[4] = RFC1048_COOKIE;

	/* a counter to manufacture transaction ids */
	static int bootp_xid = 0;

	static int starttime = 0;
	if (!starttime)
		starttime = bootp_currticks();

	/* prepare bootp packet */
	memset(&bp, 0, sizeof(struct bootp_t));
	bp.bp_op = BOOTP_REQUEST;
	bp.bp_htype = 1;
	bp.bp_hlen = ETHER_ADDR_SIZE;
	bp.bp_xid = ++bootp_xid;
	strncpy(bp.bp_vend, rfc1048_cookie, 4);	/* request RFC1048 info */
	bp.bp_vend[4] = RFC1048_END;

	/* bootp_timeout is in milliseconds, scale it to ticks */
	bootp_timeout *= BOOTP_TIMER_FREQ;
	bootp_timeout /= 1000;

	/* Fill in my hardware address */
	memcpy(bp.bp_hwaddr, hwaddr, ETHER_ADDR_SIZE);
	while (!bootp_reply && retry--) {
		int timeout;

		bp.bp_secs = htons((bootp_currticks() - starttime) /
				   BOOTP_TIMER_FREQ);

		/* send BOOTP request */
		bootp_udp_broadcast(bp.bp_hwaddr, srcip, 0, BOOTP_SERVER,
			sizeof(struct bootp_t), &bp);

		timeout = bootp_currticks() + bootp_timeout;
		while (timeout > bootp_currticks())
			if (bootp_reply)
				return 0;	/* success */
	}

	return bootp_reply ? 0 : OSKIT_ETIMEDOUT;
}

/*
 * Decodes the bootp packet & RFC1048 header
 * Fills in bootp_info, setting appropriate flags
 */
void
bootp_decode_packet(
	struct bootp_t *bootpreply,
	struct bootp_net_info *bootp_info)
{
	static const char rfc1048_cookie[4] = RFC1048_COOKIE;
	unsigned char *p = bootpreply->bp_vend;
	unsigned char *end = p + BOOTP_VENDOR_LEN,*q;
	oskit_u32_t temp;

	/* standard parts of the bootp packet */
	memcpy(&bootp_info->ip, bootpreply->bp_yiaddr, 
		sizeof bootp_info->ip);
	bootp_info->flags |= BOOTP_NET_IP;

	memcpy(&bootp_info->server, 
		bootpreply->bp_siaddr, sizeof bootp_info->server);
	bootp_info->flags |= BOOTP_NET_SERVER;

	temp = sizeof bootpreply->bp_sname;
	bootp_info->server_name = (char *)mustcalloc(temp, 1);
	memcpy(bootp_info->server_name, bootpreply->bp_sname, temp);
	bootp_info->flags |= BOOTP_NET_SERVER_NAME;

	/* boot filename */
	if (bootpreply->bp_file[0]) {
		int len = sizeof bootpreply->bp_file;
		bootp_info->bootfile_name = (char *)mustcalloc(len ,1);
		memcpy(bootp_info->bootfile_name, bootpreply->bp_file, len);
		bootp_info->flags |= BOOTP_NET_BOOTFILE_NAME;
	}       

	/* Decode the vendor-specific part of the packet, if we know the type */
	if (memcmp(p, rfc1048_cookie, 4) == 0) { /* RFC 1048 header */
		p += 4;
		while(p < end) {
			switch (*p) {
			case RFC1048_PAD:
				p++;
				continue;
			case RFC1048_END:
				p = end;
				continue;
		/*
		 * Note that I'm using memcpy first because 
		 * there are no guarantees about alignment.
		 * (Not an issue on x86, but...)
		 */

		 #define CASE_IP_TAG(TAG, field, FLAG) 		\
			case TAG:					\
				memcpy(&bootp_info->field, p+2, 	\
					sizeof bootp_info->field);	\
				bootp_info->flags |= FLAG;		\
				break;

			CASE_IP_TAG(RFC1048_NETMASK, netmask, 
				BOOTP_NET_NETMASK)

		 /* handle list of IP addresses tags */
		 #define CASE_MULTI_IP_TAG(TAG, field, FLAG) 		\
			case TAG:					\
				bootp_info->field.addr = 		\
					mustcalloc(TAG_LEN(p), 1);	\
				bootp_info->field.len = TAG_LEN(p) / 4; \
				memcpy(bootp_info->field.addr, p+2, 	\
					TAG_LEN(p));			\
				bootp_info->flags |= FLAG;		\
				break;

			CASE_MULTI_IP_TAG(RFC1048_GATEWAY, gateway, 
				BOOTP_NET_GATEWAY)
			CASE_MULTI_IP_TAG(RFC1048_LPR_SERVER, lpr_server, 
				BOOTP_NET_LPR_SERVER)
			CASE_MULTI_IP_TAG(RFC1048_LOG_SERVER, log_server, 
				BOOTP_NET_LOG_SERVER)
			CASE_MULTI_IP_TAG(RFC1048_DNS_SERVER, dns_server, 
				BOOTP_NET_DNS_SERVER)
			CASE_MULTI_IP_TAG(RFC1048_TIME_SERVER, time_server, 
				BOOTP_NET_TIME_SERVER)

		/* handle string tags */
		#define CASE_STRING_TAG(TAG, field, FLAG)		\
			case TAG:					\
				bootp_info->field = (char *)		\
					mustcalloc(TAG_LEN(p)+1, 1);	\
				memcpy(bootp_info->field, p+2, TAG_LEN(p)); \
				bootp_info->field[TAG_LEN(p)] = 0;	\
				bootp_info->flags |= FLAG;		\
				break;

			CASE_STRING_TAG(RFC1533_DOMAINNAME, domainname, 
				BOOTP_NET_DOMAINNAME)

			CASE_STRING_TAG(RFC1048_HOSTNAME, hostname, 
				BOOTP_NET_HOSTNAME)

			case RFC1048_TIME_OFFSET:
				memcpy(&temp, p+2, sizeof temp);
				bootp_info->time_offset = ntohl(temp);	
				bootp_info->flags |= BOOTP_NET_TIME_OFFSET;
				break;
			default:
				osenv_log(OSENV_LOG_WARNING,
					  PRF"Unknown RFC1048-tag ");
				for(q=p;q<p+2+TAG_LEN(p);q++)
					osenv_log(OSENV_LOG_WARNING, "%x ", *q);
				osenv_log(OSENV_LOG_WARNING, "\n\r");
			}
			p += TAG_LEN(p) + 2;
		}
	}
}

/*
 * free a bootp struct
 */
void
bootp_free(struct bootp_net_info *info)
{
#define FREE_STRING(field, t)	\
	if (info->field && (info->flags & t)) free(info->field);

	FREE_STRING(hostname, BOOTP_NET_HOSTNAME)
	FREE_STRING(server_name, BOOTP_NET_SERVER_NAME)
	FREE_STRING(bootfile_name, BOOTP_NET_BOOTFILE_NAME)
	FREE_STRING(domainname, BOOTP_NET_DOMAINNAME)

#define FREE_ADDR_ARRAY(field, t)			\
	if (info->field.addr && 			\
	    info->field.len > 0 && 			\
	    (info->flags & t)) free(info->field.addr);
	FREE_ADDR_ARRAY(gateway, BOOTP_NET_GATEWAY)
	FREE_ADDR_ARRAY(dns_server, BOOTP_NET_DNS_SERVER)
	FREE_ADDR_ARRAY(time_server, BOOTP_NET_TIME_SERVER)
	FREE_ADDR_ARRAY(log_server, BOOTP_NET_LOG_SERVER)
	FREE_ADDR_ARRAY(lpr_server, BOOTP_NET_LPR_SERVER)
}
