/*
 * Copyright (c) 1996, 1998, 1999, 2002 University of Utah and the Flux Group.
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
 * This is an example of the low-level network driver stuff.
 * It opens all ethernet cards and answers any pings.
 *
 * It can optionally demonstrate the netio_fanout feature if you
 * define FANOUT.
 */

#undef FANOUT

#include <oskit/types.h>
#include <stdio.h>
#include <setjmp.h>
#include <assert.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <oskit/io/bufio.h>
#ifdef FANOUT
#include <oskit/io/netio_fanout.h>
#endif
#include <oskit/dev/dev.h>
#include <oskit/dev/osenv.h>
#include <oskit/dev/net.h>
#include <oskit/dev/ethernet.h>
#include <oskit/dev/linux.h>
#include <oskit/c/strings.h>
#include <oskit/clientos.h>
#include <oskit/startup.h>
#ifdef TESTBED
#include <oskit/tmcp.h>
#endif

#include <oskit/net/ether.h>

#include "bootp.h"
#include "netinet.h"

#define MAX_DEVICES 8			/* max number of ethernet cards */
#define MAX_REPLIES 30			/* max number of pings to respond to */

static jmp_buf done;

struct etherdev {
	oskit_etherdev_t	*dev;
	oskit_netio_t	*send_nio;
	oskit_netio_t	*recv_nio;
	oskit_devinfo_t	info;
	unsigned char	haddr[OSKIT_ETHERDEV_ADDR_SIZE];
	struct in_addr  myip;	/* My IP address in host order. */
	int		ipbuf[512];
};
static struct etherdev devs[MAX_DEVICES];
static int ndev;


/* Sniffer support routines. */

static void
swapn(unsigned char a[], unsigned char b[], int n)
{
	int i;
	unsigned char t[n];

	for (i = 0; i < n; i++) {
		t[i] = a[i];
		a[i] = b[i];
		b[i] = t[i];
	}
}


/* The address we are passed is in network order. */
static char *
iptoa(unsigned long x)
{
	static char buf[4*4];		/* nnn.nnn.nnn.nnnz */
	unsigned char *p = (unsigned char *)&x;

	sprintf(buf, "%d.%d.%d.%d", p[0], p[1], p[2], p[3]);
	return buf;
}

static unsigned short
ipcksum(void *buf, int nbytes)
{
	unsigned long sum;

	sum = 0;
	while (nbytes > 1) {
		sum += *(unsigned short *)buf;
		buf += 2;
		nbytes -= 2;
	}
	/* Add in odd byte if any. */
	if (nbytes == 1)
		sum += *(char *)buf;

	sum = (sum >> 16) + (sum & 0xffff);     /* hi + lo */
	sum += (sum >> 16);                     /* add carry, if any */

	return (~sum);
}


/* our COM network interface. */

oskit_error_t
net_receive(void *data, oskit_bufio_t *b, oskit_size_t pkt_size)
{
	oskit_error_t rval = 0;
	unsigned char bcastaddr[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	static unsigned char *frame;
	struct ether_header *eth;
	oskit_bufio_t *our_buf;
	int err;
	struct etherdev *dev = (struct etherdev *)data;

	if (pkt_size > ETH_MAX_PACKET) {
		printf("%s: Hey Wally, I caught a big one! -- %d bytes\n",
		       dev->info.name, pkt_size);
		rval = OSKIT_E_DEV_BADPARAM;
		goto done;
	}

	err = oskit_bufio_map(b, (void **)&frame, 0, pkt_size);
	assert(err == 0);

	eth  = (struct ether_header *)frame;

	if (memcmp(eth->ether_dhost, &dev->haddr, sizeof dev->haddr) != 0
	    && memcmp(eth->ether_dhost, bcastaddr, sizeof bcastaddr) != 0) 
		goto done;			/* not for me */

	switch (ntohs(eth->ether_type)) {
	case ETHERTYPE_IP: {
		static int ipid = 0xb;		/* something random */
		static int count = 0;
		struct ip *ip;
		int hlen;
		struct in_addr hin;
		struct icmp *icmp;

		/* XXX deal with fragments. */

		ip = (struct ip *)(frame + sizeof(struct ether_header));
		hlen = ip->ip_hl << 2;

		/*
		 * Copy out from ip header on to ensure proper alignment
		 * on machines that are not byte addressable!
		 */
		memcpy(dev->ipbuf,
		       frame + sizeof(struct ether_header),
		       ntohs(ip->ip_len));
		ip = (struct ip *) dev->ipbuf;

		hin.s_addr = ntohl(ip->ip_src.s_addr);
		if (ipcksum(ip, hlen) != 0 || ip->ip_p != IPPROTO_ICMP)
			goto done;		/* bad cksum or not IP */

		icmp = (struct icmp *)((char *)ip + hlen);
		if (icmp->icmp_type != ICMP_ECHO
		    || ipcksum(icmp, ntohs(ip->ip_len) - hlen) != 0)
			goto done;		/* bad cksum or not ICMP_ECHO */

		/* Send the reply. */
		icmp->icmp_type = ICMP_ECHOREPLY;
		/* Another optimization.  Only the icmp type field in the reply
		 * changes, so we can use this apriori knowledge to determine
		 * the new checksum based upon the old checksum.  Eliminates
		 * touching all of the data in the packet once more */
#if 0
		icmp->icmp_cksum = 0;
		icmp->icmp_cksum = ipcksum(icmp, ntohs(ip->ip_len) - hlen);
#else
		if (icmp->icmp_cksum >= 65527)
			icmp->icmp_cksum += (unsigned short) 9;
		else
			icmp->icmp_cksum += (unsigned short) 8;
#endif

		ip->ip_id = htons(ipid++);

		/*
		 * We can't just swap the addresses,
		 * since broadcast packets didn't come to our address.
		 */
		ip->ip_dst = ip->ip_src;
		ip->ip_src.s_addr = htonl(dev->myip.s_addr);

		ip->ip_sum = 0;
		ip->ip_sum = ipcksum(ip, hlen);

		swapn(eth->ether_dhost, eth->ether_shost,
		      sizeof eth->ether_shost);

#if 0
		printf("%s: len = %d, iplen = %d hlen = %d\n", __FUNCTION__,
		       pkt_size, ntohs(ip->ip_len), hlen);
#endif
		/*
		 * Copy data back into original data buffer.
		 */
		memcpy(frame + sizeof(struct ether_header),
		       ip, ntohs(ip->ip_len));

		/* Removed this code because it's an unnecessary copy.
		 * We're already violating the semantics of the push
		 * by editing the data pushed to us _before_ we copy,
		 * so this copy is extraneous
		 */
#if 0
		our_buf = oskit_bufio_create(pkt_size);
		if (b != NULL) {
			oskit_size_t got;

			oskit_bufio_write(our_buf, frame, 0, pkt_size, &got);
			assert(got == pkt_size);
			oskit_netio_push(dev->send_nio, our_buf, pkt_size);
			oskit_bufio_release(our_buf);
		} else {
			printf("couldn't allocate bufio for ECHOREPLY\n");
		}
#else
		oskit_netio_push(dev->send_nio, b, pkt_size);
#endif

#if 0
		/* If you take out the printf the reply time is faster.
		   Icmp_seq is not necessarily in network order. */
		printf("%s: %2d: sent %d bytes to %s: icmp_seq=[%d,%d]\n",
		       dev->info.name, count,
		       ntohs(ip->ip_len) - hlen - sizeof(struct icmp),
		       iptoa(htonl(hin.s_addr)),
		       (icmp->icmp_seq >> 8) & 0xff, icmp->icmp_seq & 0xff);
#endif
#ifndef TESTBED
		/* We are done now */
		if (++count >= MAX_REPLIES)
			longjmp(done, 1);
#endif

		break;
	}
	case ETHERTYPE_ARP: {
		struct arphdr *arp;
		struct in_addr ip;

		arp = (struct arphdr *)(frame + sizeof(struct ether_header));
		ip.s_addr = ntohl(*(unsigned long *)arp->ar_tpa);

		if (ntohs(arp->ar_hrd) != ARPHRD_ETHER
		    || ntohs(arp->ar_pro) != ETHERTYPE_IP
		    || ntohs(arp->ar_op) != ARPOP_REQUEST
		    || ip.s_addr != dev->myip.s_addr)
			goto done;		/* wrong proto or addr */

		/* Send the reply. */
		arp->ar_op = htons(ARPOP_REPLY);
		swapn(arp->ar_spa, arp->ar_tpa, sizeof arp->ar_tpa);
		swapn(arp->ar_sha, arp->ar_tha, sizeof arp->ar_tha);
		memcpy(arp->ar_sha, dev->haddr, sizeof dev->haddr);

		/* Fill in the ethernet addresses. */
		bcopy(&eth->ether_shost, &eth->ether_dhost, sizeof(dev->haddr));
		bcopy(&dev->haddr, &eth->ether_shost, sizeof(dev->haddr));

		our_buf = oskit_bufio_create(pkt_size);
		if (b != NULL) {
			oskit_size_t got;

			oskit_bufio_write(our_buf, frame, 0, pkt_size, &got);
			assert(got == pkt_size);
			oskit_netio_push(dev->send_nio, our_buf, pkt_size);
			oskit_bufio_release(our_buf);
		} else {
			printf("couldn't allocate bufio for ARP reply\n");
		}

		printf("%s: sent ARP reply to %s\n",
		       dev->info.name, iptoa(*(unsigned long *)arp->ar_tpa));
		break;
	}
	default:
		break;
	}

done:
	return rval;
}

#ifdef FANOUT
oskit_error_t
sniff(void *data, oskit_bufio_t *b, oskit_size_t pkt_size)
{
	oskit_error_t rc = 0;
	oskit_error_t err;
	static unsigned char *frame;

	err = oskit_bufio_map(b, (void **)&frame, 0, pkt_size);
	assert(err == 0);
	printf("Sniffer received packet:\n");
	hexdumpb(0, frame, 32);

	err = oskit_bufio_unmap(b, (void *)frame, 0, pkt_size);
	assert(err == 0);
	return rc;
}
#endif /* FANOUT */


/* Stuff to get our IP address. */

/*
 * Figure out my IP address.
 */
static void
whoami(int ifnum, oskit_etherdev_t *dev, struct in_addr *myip)
{
	struct in_addr ip;

#ifdef TESTBED
	char *ipaddr = 0;
	start_tmcp_getinfo_if(ifnum, &ipaddr, 0);
	if (ipaddr)
		printf("eth%d: ipaddr=%s\n", ifnum, ipaddr);
#else
	char ipaddr[4*4];			/* nnn.nnn.nnn.nnnz */
	get_ipinfo(dev, ipaddr, 0, 0, 0);
#endif
	inet_aton(ipaddr, &ip);
	myip->s_addr = ntohl(ip.s_addr);
}


int
main(int argc, char **argv)
{
	oskit_error_t err;
	oskit_etherdev_t **etherdev;
	int i;
#ifdef FANOUT
	oskit_netio_fanout_t *fnetio_in, *fnetio_out;
	oskit_netio_t  *sniffer;
#endif /* FANOUT */

	/* Must be on little endian machine. */
	i = 1;
	assert(*(char *)&i == 1);

#ifndef KNIT
	oskit_clientos_init();
#endif
#ifdef  GPROF
	start_fs_bmod();
	start_gprof();
#endif
#ifndef KNIT
#ifdef TESTBED
	start_tmcp(0);
#else
	start_net_devices();
#endif
#endif

	/*
	 * Find all the Ethernet device nodes.
	 */
	ndev = osenv_device_lookup(&oskit_etherdev_iid, (void***)&etherdev);
	if (ndev <= 0)
		panic("no Ethernet adaptors found!");
	if (ndev > MAX_DEVICES)
		panic("XXX too many Ethernet adaptors found");

	/*
	 * Open all the devices and have them hand packets to the OS.
	 */
	printf("%d Ethernet adaptor%s found:\n", ndev, ndev > 1 ? "s" : "");
	for (i = 0; i < ndev; i++) {
		int j;

		err = oskit_etherdev_getinfo(etherdev[i], &devs[i].info);
		if (err)
			panic("error getting info from ethercard %d", i);

		oskit_etherdev_getaddr(etherdev[i], devs[i].haddr);

		/* Show information about this adaptor */
		printf("  %-16s%-40s ", devs[i].info.name,
			devs[i].info.description
			? devs[i].info.description : "");
		for (j = 0; j < 5; j++) 
			printf("%02x:", devs[i].haddr[j]);
		printf("%02x\n", devs[i].haddr[5]);

		/* get the IP address */
		whoami(i, etherdev[i], &devs[i].myip);

		devs[i].dev = etherdev[i];
		devs[i].recv_nio = oskit_netio_create(net_receive, &devs[i]);
		if (devs[i].recv_nio == NULL)
			panic("unable to create recv_nio");

#ifndef FANOUT
		err = oskit_etherdev_open(etherdev[i], 0, devs[i].recv_nio,
					  &devs[i].send_nio);
#else
		/* Create additional listener. */
		sniffer = oskit_netio_create(sniff, NULL);

		/* Create fanout for incoming packets and add it's listeners. */
		fnetio_in = oskit_netio_fanout_create();
		oskit_netio_fanout_add_listener(fnetio_in, devs[i].recv_nio);
		oskit_netio_fanout_add_listener(fnetio_in, sniffer);

		/*
		 * Open this adaptor and have incoming packets go to
		 * the fanout instead of straight to recv_nio.
		 */
		err = oskit_etherdev_open(etherdev[i], 0, 
					  (oskit_netio_t *)fnetio_in,
					  &devs[i].send_nio);
		if (err)
			panic("Error opening ethercard %d (rc=%p)", i, err);

		/* Create fanout for outgoing packets and add it's listeners. */
		fnetio_out = oskit_netio_fanout_create();
		oskit_netio_fanout_add_listener(fnetio_out, devs[i].send_nio);
		oskit_netio_fanout_add_listener(fnetio_out, sniffer);

		/*
		 * Set the send_nio to go to the fanout instead of
		 * straight to the card.
		 */
		oskit_netio_release(devs[i].send_nio);
		devs[i].send_nio = (oskit_netio_t *)fnetio_out;
#endif /* FANOUT */
	}

	if (!osenv_intr_enabled()) {
		printf("Interrupts not enabled! enabling!\n");	
		osenv_intr_enable();
	}

#ifdef TESTBED
	printf("Ready for pings, ^C to exit...\n");
#else
	printf("ping me, I dare you -- I will quit when I get %d pings\n",
		       MAX_REPLIES);
#endif
	if (! setjmp(done))
		while (1) {
#ifdef TESTBED
			int ch = com_cons_trygetchar(1);
			if (ch == 3)
				longjmp(done, 1);
#endif
			continue;
		}
	printf("ALL DONE\n");
	for (i=0; i < ndev; i++) {
		oskit_etherdev_release(devs[i].dev);
		oskit_netio_release(devs[i].send_nio);
		oskit_netio_release(devs[i].recv_nio);
	}

	exit(0);
}

#ifdef TESTBED
/*
 * Don't annoy us with babble
 */
void
osenv_vlog(int priority, const char *fmt, void *args)
{
#ifndef DEBUG
	if (priority > OSENV_LOG_ERR)
		return;
#endif

        vprintf(fmt, args);
}
void
osenv_log(int priority, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	osenv_vlog(priority, fmt, args);
	va_end(args);
}

/*
 * We only configure interfaces used in the testbed
 */
void
start_net_devices(void)
{
	oskit_osenv_t *osenv = start_osenv();

	oskit_dev_init(osenv);
	oskit_linux_init_osenv(osenv);
	oskit_linux_init_ethernet_eepro100();
	oskit_linux_init_ethernet_vortex();
	oskit_linux_init_ethernet_tulip();
	oskit_dev_probe();
}
#endif
