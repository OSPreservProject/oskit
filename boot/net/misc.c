/*
 * Copyright (c) 1997, 2000, 2001 University of Utah and the Flux Group.
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

#include <sys/types.h>
#include <netinet/in.h>		/* INADDR_NONE */
#include <arpa/inet.h>		/* htonl */
#include <netdb.h>		/* gethostbyname */
#include <string.h>		/* memcpy */
#include <strings.h>		/* strsep */
#include <stdarg.h>		/* va_list */
#include <stdio.h>		/* putchar */

#include "boot.h"		/* XPORT_* */
#include "misc.h"

/*
 * Convert IP address from net to machine order
 */
void
convert_ipaddr(void *dv, void *sv)
{
	char *d = dv, *s = sv;

	*(d+3) = *s;
	*(d+2) = *(s+1);
	*(d+1) = *(s+2);
	*d     = *(s+3);
}

/**************************************************************************
PRINTF and friends

	Formats:
		%X	- 4 byte ASCII (8 hex digits)
		%x	- 2 byte ASCII (4 hex digits)
		%b	- 1 byte ASCII (2 hex digits)
		%d	- decimal
		%c	- ASCII char
		%s	- ASCII string
		%I	- Internet address in x.x.x.x notation
		%L	- Binary long
		%S	- String (multiple of 4 bytes) preceded with 4 byte
			  binary length
		%M	- Copy memory.  Takes two args, len and ptr
**************************************************************************/
static char *
do_netprintf(char *buf, char *fmt, va_list args)
{
	static char hex[]="0123456789ABCDEF";
	register char *p;
	char tmp[16];

	while (*fmt) {
		if (*fmt == '%') {	/* switch() uses more space */
			fmt++;
			if (*fmt == 'L') {
				register int h = va_arg(args, int);
				*(buf++) = h>>24;
				*(buf++) = h>>16;
				*(buf++) = h>>8;
				*(buf++) = h;
			}
			if (*fmt == 'S') {
				register int len = 0;
				char *lenptr = buf;
				p = va_arg(args, char *);
				buf += 4;
				while (*p) {
					*(buf++) = *p++;
					len ++;
				}
				*(lenptr++) = len>>24;
				*(lenptr++) = len>>16;
				*(lenptr++) = len>>8;
				*lenptr = len;
				while (len & 3) {
					*(buf++) = 0;
					len ++;
				}
			}
			if (*fmt == 'M') {
				register int len = va_arg(args, int);
				memcpy(buf, va_arg(args, void *), len);
				buf += len;
			}
			if (*fmt == 'X') {
				register int h = va_arg(args, int);
				*(buf++) = hex[(h>>28)& 0x0F];
				*(buf++) = hex[(h>>24)& 0x0F];
				*(buf++) = hex[(h>>20)& 0x0F];
				*(buf++) = hex[(h>>16)& 0x0F];
				*(buf++) = hex[(h>>12)& 0x0F];
				*(buf++) = hex[(h>>8)& 0x0F];
				*(buf++) = hex[(h>>4)& 0x0F];
				*(buf++) = hex[h& 0x0F];
			}
			if (*fmt == 'x') {
				register int h = va_arg(args, int);
				*(buf++) = hex[(h>>12)& 0x0F];
				*(buf++) = hex[(h>>8)& 0x0F];
				*(buf++) = hex[(h>>4)& 0x0F];
				*(buf++) = hex[h& 0x0F];
			}
			if (*fmt == 'b') {
				register int h = va_arg(args, int);
				*(buf++) = hex[(h>>4)& 0x0F];
				*(buf++) = hex[h& 0x0F];
			}
			if (*fmt == 'd') {
				register int dec = va_arg(args, int);
				p = tmp;
				if (dec < 0) {
					*(buf++) = '-';
					dec = -dec;
				}
				do {
					*(p++) = '0' + (dec%10);
					dec = dec/10;
				} while(dec);
				while ((--p) >= tmp) *(buf++) = *p;
			}
			if (*fmt == 'I') {
				register int i = va_arg(args, int);
				buf = netsprintf(buf,"%d.%d.%d.%d",
					(i>>24) & 0x00FF,
					(i>>16) & 0x00FF,
					(i>>8) & 0x00FF,
					i & 0x00FF);
			}
			if (*fmt == 'c')
				*(buf++) = va_arg(args, int);
			if (*fmt == 's') {
				p = va_arg(args, char *);
				while (*p) *(buf++) = *p++;
			}
		} else *(buf++) = *fmt;
		fmt++;
	}
	va_end(args);
	*buf = 0;
	return(buf);
}

char *
netsprintf(char *buf, char *fmt, ...)
{
	va_list args;
	char *r;

	va_start(args, fmt);
	r = do_netprintf(buf, fmt, args);
	va_end(args);
	return r;
}

void
netprintf(char *fmt, ...)
{
	va_list args;
	char buf[512], *p;

	va_start(args, fmt);
	p = buf;
	do_netprintf(buf, fmt, args);
	while (*p) putchar(*p++);
	va_end(args);
}

/*
 * Hostname lookup. First try the resolver, and then fall back to the table.
 */
static unsigned long
hostlookup(const char *host_or_ip)
{
	struct hostent *he;
	int i;
	struct hostmap {
		const char* host;
		const char* ip;
	} hostmap[] = {
		/* Host to IP map.  Keep in alpha order.  These are
		 * padded with NULs so the binary can be patched...
		 */
#ifdef UTAHTESTBED
		{ "boss",    "155.101.128.70\0" },
		{ "fs",      "155.101.129.74\0" },
		{ "ns",      "155.101.128.70\0" },
		{ "ops",     "155.101.129.74\0" },
		{ "paper",   "155.101.128.70\0" },
		{ "plastic", "155.101.129.74\0" },
#else
		{ "alta",    "155.99.212.80\0\0" },
		{ "axl",     "155.99.212.112\0" },
		{ "fast",    "155.99.212.1\0\0\0" },
		{ "ibapah",  "155.99.212.83\0\0" },
		{ "irontown","155.99.212.85\0\0" },
		{ "marker",  "155.99.212.61\0\0" },
		{ "moab",    "155.99.212.88\0\0" },
		{ "thistle", "155.99.212.90\0\0" },
#endif
	};

	he = gethostbyname(host_or_ip);
	if (he) {
		struct in_addr in;
			
		memcpy ((char *)&in, he->h_addr, he->h_length);

		return ntohl(in.s_addr);
	}

	for (i = 0; i < sizeof(hostmap)/sizeof(hostmap[0]); i++)
	{
		if (!strcmp(host_or_ip, hostmap[i].host))
		{
			host_or_ip = hostmap[i].ip;
			
			return ntohl(inet_addr(host_or_ip));
		}
	}
	return INADDR_NONE;
}

static int
transportlookup(char *proto)
{
	int i;
	struct protos {
		char *name;
		int value;
	} xportmap[] = {
		{ "nfs",	XPORT_NFS },
		{ "tftp",	XPORT_TFTP },
	};

	if (proto && *proto)
	{
		for (i = 0; i < sizeof(xportmap)/sizeof(xportmap[0]); i++)
		{
			if (!strcmp(proto, xportmap[i].name))
				return xportmap[i].value;
		}
	}
	return XPORT_NFS;
}

/*
 * Take the command line and set the IP, dir, and name components.
 * The cmdline is modified and things point into it.
 * The cmdline looks like "[proto@]ip:/dir/file -foo bar",
 * eg "155.99.212.1:/tmp/foo -l" or "tftp@155.99.212.1:/tftpboot/foo"
 */
int
parse_cmdline(char *cmdline, int cmdsize, unsigned int *xport, unsigned int *ip,
	      char **dir, char **name, char **rest)
{
	char *p = cmdline;
	char *host_or_ip, *tproto;

	p = strsep(&cmdline, " \t");
	*rest = cmdline;		/* ok if NULL */

	host_or_ip = strsep(&p, ":");
	if (! p)
		return 0;

	tproto = strsep(&host_or_ip, "@");
	if (! host_or_ip)
	{
		host_or_ip = tproto;
		tproto = NULL;
	}
	*xport = transportlookup(tproto);

	*ip = hostlookup(host_or_ip);
	if (*ip == INADDR_NONE)
		return 0;

	if (p[0] != '/')
		return 0;
	*dir = p;

	p = strrchr(*dir, '/');
	if (! p)
		return 0;
	*p++ = '\0';
	if (! *p)
		return 0;
	*name = p;

	/* Allow the case ip:/foo; we might have just NULified that slash. */
	if ((*dir)[0] == '\0')
		*dir = "/";

	return 1;
}
