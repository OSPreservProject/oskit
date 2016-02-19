/*
 * Copyright (c) 1997 M.I.T.
 * All rights reserved.
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
 *      This product includes software developed by MIT.
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


/* protocol library */
#include <old-dpf.h>
#include <dpf_lib.h>

/* Unconnected UDP Filter */

static struct dpf_frozen_ir udpu[] = {
	/* op              len     r0      r1      imm     */
	{DPF_EQI, 2, 1, 0, 0x8},/* 0 */
	{DPF_BITS16I, 0, 0, 0, 0xc},	/* 1 */
	{DPF_EQI, 2, 1, 0, 0x45},	/* 3 */
	{DPF_BITS8I, 0, 0, 0, 0xe},	/* 4 */
	{DPF_EQI, 2, 1, 0, 0x11},	/* 5 */
	{DPF_BITS8I, 0, 0, 0, 0x17},	/* 6 */

	{DPF_EQI, 2, 1, 0, 0x12},	/* 15 */
	{DPF_BITS8I, 0, 0, 0, 0x1e},	/* 16 */
	{DPF_EQI, 2, 1, 0, 0x1a},	/* 17 */
	{DPF_BITS8I, 0, 0, 0, 0x1f},	/* 18 */
	{DPF_EQI, 2, 1, 0, 0x00},	/* 19 */
	{DPF_BITS8I, 0, 0, 0, 0x20},	/* 20 */
	{DPF_EQI, 2, 1, 0, 0x88},	/* 21 */
	{DPF_BITS8I, 0, 0, 0, 0x21},	/* 22 */

	{DPF_EQI, 2, 1, 0, 0x8813},	/* 26 */
	{DPF_BITS16I, 0, 0, 0, 0x24},	/* 27 */
};
/* make a udp packet-filter */
void   *
mk_udpu(int *sz, unsigned short src_port, unsigned char *src_ip)
{
	udpu[6].imm = src_ip[0];
	udpu[8].imm = src_ip[1];
	udpu[10].imm = src_ip[2];
	udpu[12].imm = src_ip[3];

	udpu[14].imm = src_port;
	*sz = sizeof udpu / sizeof udpu[0];
	return (void *) udpu;
}
/* Connected UDP filter */

static struct dpf_frozen_ir udpc[] = {
	/* op              len     r0      r1      imm     */
	{DPF_EQI, 2, 1, 0, 0x8},/* 0 */
	{DPF_BITS16I, 0, 0, 0, 0xc},	/* 1 */
	{DPF_EQI, 2, 1, 0, 0x45},	/* 3 */
	{DPF_BITS8I, 0, 0, 0, 0xe},	/* 4 */
	{DPF_EQI, 2, 1, 0, 0x11},	/* 5 */
	{DPF_BITS8I, 0, 0, 0, 0x17},	/* 6 */

	{DPF_EQI, 2, 1, 0, 0x12},	/* 7 */
	{DPF_BITS8I, 0, 0, 0, 0x1a},	/* 8 */
	{DPF_EQI, 2, 1, 0, 0x1a},	/* 9 */
	{DPF_BITS8I, 0, 0, 0, 0x1b},	/* 10 */
	{DPF_EQI, 2, 1, 0, 0x00},	/* 11 */
	{DPF_BITS8I, 0, 0, 0, 0x1c},	/* 12 */
	{DPF_EQI, 2, 1, 0, 0x54},	/* 13 */
	{DPF_BITS8I, 0, 0, 0, 0x1d},	/* 14 */

	{DPF_EQI, 2, 1, 0, 0x12},	/* 15 */
	{DPF_BITS8I, 0, 0, 0, 0x1e},	/* 16 */
	{DPF_EQI, 2, 1, 0, 0x1a},	/* 17 */
	{DPF_BITS8I, 0, 0, 0, 0x1f},	/* 18 */
	{DPF_EQI, 2, 1, 0, 0x00},	/* 19 */
	{DPF_BITS8I, 0, 0, 0, 0x20},	/* 20 */
	{DPF_EQI, 2, 1, 0, 0x82},	/* 21 */
	{DPF_BITS8I, 0, 0, 0, 0x21},	/* 22 */

	{DPF_EQI, 2, 1, 0, 0xd00},	/* 24 */
	{DPF_BITS16I, 0, 0, 0, 0x22},	/* 25 */
	{DPF_EQI, 2, 1, 0, 0x8813},	/* 26 */
	{DPF_BITS16I, 0, 0, 0, 0x24},	/* 27 */
};



void   *
mk_udpc(int *sz,
    unsigned short src_port, unsigned char *src_ip,
    unsigned short dst_port, unsigned char *dst_ip)
{
	udpc[6].imm = dst_ip[0];
	udpc[8].imm = dst_ip[1];
	udpc[10].imm = dst_ip[2];
	udpc[12].imm = dst_ip[3];

	udpc[14].imm = src_ip[0];
	udpc[16].imm = src_ip[1];
	udpc[18].imm = src_ip[2];
	udpc[20].imm = src_ip[3];

	udpc[22].imm = dst_port;
	udpc[24].imm = src_port;
	*sz = sizeof udpc / sizeof udpc[0];
	return (void *) udpc;
}


/* RARP FILTER */

static struct dpf_frozen_ir rarp[] = {
	/* op              len     r0      r1      imm     */
	{DPF_EQI, 2, 1, 0, 0x3580},	/* Protocol RARP */
	{DPF_BITS16I, 0, 0, 0, 0xc},	/* 1 */
	{DPF_EQI, 2, 1, 0, 0x1},/* HW proto */
	{DPF_BITS8I, 0, 0, 0, 0xf},	/* 4 */
	{DPF_EQI, 2, 1, 0, 0x08},	/* Target proto */
	{DPF_BITS16I, 0, 0, 0, 0x10},	/* 6 */
	{DPF_EQI, 2, 1, 0, 0x400},	/* Operation */
	{DPF_BITS16I, 0, 0, 0, 0x14},	/* 8 */
	{DPF_EQI, 2, 1, 0, 0x0},/* HW addr byte 0 */
	{DPF_BITS8I, 0, 0, 0, 0x20},	/* 11 */
	{DPF_EQI, 2, 1, 0, 0x0},/* HW addr byte 1 */
	{DPF_BITS8I, 0, 0, 0, 0x21},	/* 13 */
	{DPF_EQI, 2, 1, 0, 0x0},/* HW addr byte 2 */
	{DPF_BITS8I, 0, 0, 0, 0x22},	/* 15 */
	{DPF_EQI, 2, 1, 0, 0x0},/* HW addr byte 3 */
	{DPF_BITS8I, 0, 0, 0, 0x23},	/* 17 */
	{DPF_EQI, 2, 1, 0, 0x0},/* HW addr byte 4 */
	{DPF_BITS8I, 0, 0, 0, 0x24},	/* 19 */
	{DPF_EQI, 2, 1, 0, 0x0},/* HW addr byte 5 */
	{DPF_BITS8I, 0, 0, 0, 0x25},	/* 21 */
};

struct elem *
mk_rarp(int *sz, unsigned char *ethaddr)
{
	rarp[8].imm = ethaddr[0];
	rarp[10].imm = ethaddr[1];
	rarp[12].imm = ethaddr[2];
	rarp[14].imm = ethaddr[3];
	rarp[16].imm = ethaddr[4];
	rarp[18].imm = ethaddr[5];
	*sz = sizeof rarp / sizeof rarp[0];
	return (void *) rarp;
}


/* ARP FILTER */
static struct dpf_frozen_ir arp[] = {
	/* op              len     r0      r1      imm     */
	{DPF_EQI, 2, 1, 0, 0x0608},	/* Protocol ARP */
	{DPF_BITS16I, 0, 0, 0, 0xc},	/* 1 */
	{DPF_EQI, 2, 1, 0, 0x1},/* HW proto */
	{DPF_BITS8I, 0, 0, 0, 0xf},	/* 4 */
	{DPF_EQI, 2, 1, 0, 0x08},	/* Target proto */
	{DPF_BITS16I, 0, 0, 0, 0x10},	/* 6 */
/* we tend to see more arp requests than replies */
#ifdef OSKIT
	{DPF_EQI, 2, 1, 0, 0x100},	/* Operation arp request */
#else
	{DPF_EQI, 2, 1, 0, 0x200},	/* Operation = arp reply */
#endif
	{DPF_BITS16I, 0, 0, 0, 0x14},	/* 8 */
};
struct elem *
mk_arp(int *sz, unsigned char *ipaddr)
{
	*sz = sizeof arp / sizeof arp[0];
	return (void *) arp;
}


/* CUSTOM PROTOCOL, very cheap, just allocate our own ether protocol, and the first
   short specifies the stream number (this is an example for a video server,
   point-to-point same ethernet) */

static struct dpf_frozen_ir custom[] = {
	/* op              len     r0      r1      imm     */
	{DPF_EQI, 2, 1, 0, 0xbad},	/* Protocol CUSTOM */
	{DPF_BITS16I, 0, 0, 0, 0xc},	/* 1 */
	{DPF_EQI, 2, 1, 0, 0x1},/* Stream Number */
	{DPF_BITS16I, 0, 0, 0, 0xe},	/* 3 */
};
struct elem *
mk_custom(int *sz, unsigned short stream_no)
{
	custom[2].imm = stream_no;
	*sz = sizeof custom / sizeof custom[0];
	return (void *) custom;
}


/* ARP STRING */
/* The filter for ARP packets should match them all, since we do the final
   filtering in the ARP client, the reason is that we may want to cache other ARP
   messages that are in the network. */
static unsigned char arp_string[] = {
	/* Ethernet Layer */
	0, 0, 0, 0, 0, 0,	/* ether destination */
	0, 0, 0, 0, 0, 0,	/* ether source */
	0x08, 0x06,		/* packet type ie ARP */

	/* ARP Layer */
	0, 1,			/* Hardware Protocol */
	0x08, 0x00,		/* Target Protocol */
	6, 4,			/* Har Len, Tar Len */
/* we tend to see more arp requests than replies */
#ifdef OSKIT
	0, 1, 		        /* Operation (query) */
#else
	0, 2,			/* Operation (response to query) */
#endif
	0, 0, 0, 0, 0, 0,	/* Sender Hardware (ether) */
	0, 0, 0, 0,		/* Sender Target (IP) */
	0, 0, 0, 0, 0, 0,	/* Response Hardware */
	0, 0, 0, 0		/* Response IP */
};

void 
mk_arp_string(char *new, unsigned char *ipaddr)
{
	int     sz = (sizeof arp_string);
	memcpy(new, arp_string, sz);
	memcpy(new + 26, ipaddr, 4);
}
/* RARP STRING */
static unsigned char rarp_string[] = {
	/* Ethernet Layer */
	0, 0, 0, 0, 0, 0,	/* ether destination */
	0, 0, 0, 0, 0, 0,	/* ether source */
	0x80, 0x35,		/* packet type ie RARP */

	/* RARP Layer */
	0, 1,			/* Hardware Protocol */
	0x08, 0x00,		/* Target Protocol */
	6, 4,			/* Har Len, Tar Len */
	0, 4,			/* Operation (response to RARP) */
	0, 0, 0, 0, 0, 0,	/* Sender Hardware (ether) */
	0, 0, 0, 0,		/* Sender Target (IP) */
	0, 0, 0, 0, 0, 0,	/* Response Hardware */
	0, 0, 0, 0		/* Response IP */
};

void
mk_rarp_string(char  *new, unsigned char *ethaddr)
{
	memcpy(new, rarp_string, sizeof rarp_string);
	memcpy(new + 32, ethaddr, 6);
}

/* UDP STRING */
static unsigned char udpc_string[] = {
	/* Ethernet Layer */
	0, 0, 0, 0, 0, 0,	/* ether destination */
	0, 0, 0, 0, 0, 0,	/* ether source */
	0x08, 0x00,		/* packet type ie IP */

	/* IP Layer */
	0x45, 0,		/* IP vers, type of service */
	0, 0,			/* total length */
	0, 0, 0, 0,		/* Ident and Fragment */
	0, 0x11,		/* TTL, protocol ie UDP */
	0, 0,			/* header checksum */
	18, 26, 0, 89,		/* source IP addr */
	18, 26, 0, 136,		/* dest IP addr */

	/* UDP Layer */
	0, 13,			/* source UDP port */
	0x13, 0x88,		/* dest   UDP port */
	0, 0,			/* UDP length */
	0, 0			/* UDP checksum */
};

void 
mk_udpc_string(char *new,
    unsigned short src_port, unsigned char *src_ip,
    unsigned short dst_port, unsigned char *dst_ip)
{
	int     sz = sizeof udpc_string;

	memcpy(new, udpc_string, sz);
	memcpy(new + 26, dst_ip, 4);
	memcpy(new + 30, src_ip, 4);
	memcpy(new + 34, (unsigned char *) &dst_port, 2);
	memcpy(new + 36, (unsigned char *) &src_port, 2);
}




static struct dpf_frozen_ir tcp_connected[] = {
        /*      op              len     r0      r1      imm     */
        {       DPF_EQI,        2,      1,      0,      0x8}, /* 0 */
        {       DPF_BITS16I,    0,      0,      0,      0xc}, /* 1 */
        {       DPF_EQI,        2,      1,      0,      0x6}, /* 2 */
        {       DPF_BITS8I,     0,      0,      0,      0x17}, /* 3 */
        {       DPF_EQI,        2,      1,      0,      0x1a12}, /* 4 */
        {       DPF_BITS16I,    0,      0,      0,      0x1a}, /* 5 */
        {       DPF_EQI,        2,      1,      0,      0x900}, /* 6 */
        {       DPF_BITS16I,    0,      0,      0,      0x1c}, /* 7 */
        {       DPF_EQI,        2,      1,      0,      0x9019}, /* 8 */
        {       DPF_BITS16I,    0,      0,      0,      0x22}, /* 9 */
        {       DPF_EQI,        2,      1,      0,      0x50d}, /* 10 */
        {       DPF_BITS16I,    0,      0,      0,      0x24}, /* 11 */
};


static struct dpf_frozen_ir tcp_listen[] = {
        /*      op              len     r0      r1      imm     */
        {       DPF_EQI,        2,      1,      0,      0x8}, /* 0 */
        {       DPF_BITS16I,    0,      0,      0,      0xc}, /* 1 */
        {       DPF_EQI,        2,      1,      0,      0x6}, /* 2 */
        {       DPF_BITS8I,     0,      0,      0,      0x17}, /* 3 */
        {       DPF_EQI,        2,      1,      0,      0x50d}, /* 4 */
        {       DPF_BITS16I,    0,      0,      0,      0x24}, /* 5 */
};


void *mk_tcp_short(int *sz, short dst_port)  {
	*sz = sizeof tcp_listen / sizeof tcp_listen[0];
	tcp_listen[4].imm = dst_port;
	return (void *)tcp_listen;
}

void *mk_tcp_long(int *sz, unsigned short src_port, unsigned short srcip1, 
		  unsigned short dst_port, unsigned short srcip2)
{
	*sz = sizeof tcp_connected / sizeof tcp_connected[0];

	tcp_connected[4].imm = srcip1; 
	tcp_connected[6].imm = srcip2;

	tcp_connected[8].imm = src_port;
	tcp_connected[10].imm = dst_port;

	return (void *)tcp_connected;
}


/* TCP STRING */
static unsigned char tcpc_string[] = {
	/* Ethernet Layer */
	0, 0, 0, 0, 0, 0,	/* ether destination */
	0, 0, 0, 0, 0, 0,	/* ether source */
	0x8, 0x00,		/* packet type ie IP */		/* 14 */

	/* IP Layer */
	0x45, 0,		/* IP vers, type of service */
	0, 0,			/* total length */		/* 18 */
	0, 0, 0, 0,		/* Ident and Fragment */
	0, 0x6,			/* TTL, protocol ie UDP */
	0, 0,			/* header checksum */
	18, 26, 0, 9,		/* source IP addr */
	18, 26, 0, 136,		/* dest IP addr */

	/* TCP Layer */
	0x90, 0x19,		/* source port */
	0x50, 0xd,		/* dest port */
	0, 0, 0, 0,		/* seqno */
	0, 0, 0, 0,		/* ackno */
	0, 			/* offset */
	0x10,			/* flags */
	0, 0,			/* windown */
	0, 0,			/* cksum */
	0, 0,			/* urgent ptr */
};


void 
mk_tcp_string(char *new,
    unsigned short src_port, unsigned short srcip1,
    unsigned short dst_port, unsigned short srcip2)
{
	int     sz = sizeof tcpc_string;

	memcpy(new, tcpc_string, sz);
	memcpy(new + 26, &srcip1, 2);
	memcpy(new + 28, &srcip2, 2);
	memcpy(new + 34, (unsigned char *) &src_port, 2);
	memcpy(new + 36, (unsigned char *) &dst_port, 2);
}


