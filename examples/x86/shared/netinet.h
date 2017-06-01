/* Struct defs hacked from netinet headers. */
/*
 * Copyright (c) 1982, 1986, 1993
 *      The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef __NETINET_H_INCLUDED__
#define __NETINET_H_INCLUDED__

struct	arphdr {
	unsigned short	ar_hrd;		/* format of hardware address */
#define ARPHRD_ETHER 	1		/* ethernet hardware format */
	unsigned short	ar_pro;		/* format of protocol address */
	unsigned char	ar_hln;		/* length of hardware address */
	unsigned char	ar_pln;		/* length of protocol address */
	unsigned short	ar_op;		/* one of: */
#define	ARPOP_REQUEST	1		/* request to resolve address */
#define	ARPOP_REPLY	2		/* response to previous request */
	unsigned char	ar_sha[6];	/* sender hardware address (ethernet) */
	unsigned char	ar_spa[4];	/* sender protocol address (IP) */
	unsigned char	ar_tha[6];	/* target hardware address (ethernet) */
	unsigned char	ar_tpa[4];	/* target protocol address (IP) */
};

struct real_arphdr {
	unsigned short	ar_hrd;		/* format of hardware address */
	unsigned short	ar_pro;		/* format of protocol address */
	unsigned char	ar_hln;		/* length of hardware address */
	unsigned char	ar_pln;		/* length of protocol address */
	unsigned short	ar_op;		/* one of: */
};

struct ether_arp {
	struct real_arphdr ea_hdr;      /* fixed-size header */
	unsigned char      arp_sha[6];  /* sender hardware address (ethernet) */
	unsigned char      arp_spa[4];  /* sender protocol address (IP) */
	unsigned char      arp_tha[6];  /* target hardware address (ethernet) */
	unsigned char      arp_tpa[4];  /* target protocol address (IP) */
};

#define arp_hrd ea_hdr.ar_hrd
#define arp_pro ea_hdr.ar_pro
#define arp_hln ea_hdr.ar_hln
#define arp_pln ea_hdr.ar_pln
#define arp_op  ea_hdr.ar_op

struct ip {
	/* Little endian. */
        unsigned char	ip_hl:4,	/* header length */
			ip_v:4;		/* version */
        unsigned char	ip_tos;		/* type of service */
        short		ip_len;		/* total length */
        unsigned short	ip_id;		/* identification */
        short		ip_off;		/* fragment offset field */
        unsigned char	ip_ttl;		/* time to live */
        unsigned char	ip_p;		/* protocol */
#define IPPROTO_ICMP 1
        unsigned short	ip_sum;		/* checksum */
        struct	in_addr ip_src,ip_dst;  /* source and dest address */
};

struct icmp {
        unsigned char	icmp_type;	/* type of message, see below */
#define ICMP_ECHO 8
#define ICMP_ECHOREPLY 0
        unsigned char	icmp_code;	/* type sub code */
        unsigned short	icmp_cksum;	/* ones complement cksum of struct */
	unsigned short	icmp_id;
	unsigned short	icmp_seq;
};

#endif /* __NETINET_H_INCLUDED__ */
