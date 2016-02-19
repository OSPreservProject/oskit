/*
 * Copyright (c) 1998, 2001 The University of Utah and the Flux Group.
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
 * A simple UDP interface without fragmentation.
 */

#include <oskit/dev/dev.h>
#include <oskit/error.h>
#include <oskit/net/ether.h>
#include <oskit/net/socket.h>
#include <oskit/dev/ethernet.h>
#include <oskit/io/asyncio.h>
#include <oskit/io/bufio.h>
#include <oskit/com/listener_mgr.h>
#include <oskit/c/netinet/in.h>
#include <oskit/c/netinet/udp.h>
#include <oskit/c/netinet/ip.h>
#include <oskit/c/netinet/udp.h>
#include <oskit/c/arpa/inet.h>
#include <oskit/c/stdio.h>
#include <oskit/c/string.h>
#include <oskit/c/stdlib.h>
#include <oskit/c/assert.h>
#include <oskit/net/ether.h>
#include <oskit/com/queue.h>
#include <oskit/queue.h>

#include <oskit/udplib.h>
#include "udplib.h"

/* Queue of all open sockets */
static queue_head_t		fudp_sockets;

/* Local port numbering */
static oskit_u16_t		nextport = 3000;

/* Size of per-socket packet queue. */
#define PACKQ_MAX		64

#define min(a,b)	((a) < (b)? (a) : (b))

/*
 * XXX This is actually not correct because
 * in COM the callee is supposed to pop args.
 */
static OSKIT_COMDECL
notimpl(void)
{
	return OSKIT_E_NOTIMPL;
}

/* Implementation of the COM socket interface for FakeUDP. */

struct sk_impl {
	oskit_socket_t	sk_ioi;
	oskit_asyncio_t	sk_ioa;
	oskit_u32_t	sk_count;

	oskit_u16_t	sk_port;	/* net order */
	struct in_addr	sk_ipaddr;	/* net order */
	oskit_queue_t  *sk_packq;	/* queue of packets */
	queue_chain_t	chain;		/* queue of all sockets. */
	char		connected;	/* Is this a "connected" socket */
	char		selecting;	/* A select in progress */
	struct listener_mgr *readers;	/* listeners for asyncio READ */

	/* Duplicated. Should be per device not socket */
	struct in_addr	sf_ipaddr;	/* net order */
	struct in_addr	sf_netmask;	/* net order */
	struct in_addr	sf_gateway;	/* net order */
};

static OSKIT_COMDECL
sk_query(oskit_socket_t *sk, const struct oskit_guid *iid, void **out_ihandle)
{
	struct sk_impl *ski = (void *)sk;

	assert(ski && ski->sk_count);

        if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
            memcmp(iid, &oskit_socket_iid, sizeof(*iid)) == 0) {
		*out_ihandle = &ski->sk_ioi;
		ski->sk_count++;
		return 0;
	}

        if (memcmp(iid, &oskit_asyncio_iid, sizeof(*iid)) == 0) {
                *out_ihandle = &ski->sk_ioa;
		ski->sk_count++;
                return 0;
	}

	*out_ihandle = NULL;
	return OSKIT_E_NOINTERFACE;
}

static OSKIT_COMDECL_U
sk_addref(oskit_socket_t *sk)
{
	struct sk_impl *ski = (void *)sk;

	assert(ski && ski->sk_count);

	return ++ski->sk_count;
}

static OSKIT_COMDECL_U
sk_release(oskit_socket_t *sk)
{
	struct sk_impl *ski = (void *)sk;
	int enabled;

	assert(ski && ski->sk_count);

	if (--ski->sk_count)
		return ski->sk_count;

	enabled = osenv_intr_save_disable();
	queue_remove(&fudp_sockets, ski, struct sk_impl *, chain);
	if (enabled)
		osenv_intr_enable();
	/* This will release queued bufios */
	oskit_queue_release(ski->sk_packq);
	oskit_destroy_listener_mgr(ski->readers);	
	free(ski);
	return 0;
}

/*
 * Assign the port and ipaddr for the socket to use.
 */
static OSKIT_COMDECL
sk_bind(oskit_socket_t *sk,
	const struct oskit_sockaddr *name,
	oskit_size_t namelen)
{
	struct sk_impl *ski = (void *)sk;
	struct sockaddr_in *sin	= (void *)name;
	struct sk_impl *skitmp;

	assert(ski && ski->sk_count);

	if (sin->sin_family != OSKIT_AF_INET)
		return OSKIT_E_INVALIDARG;

	if (sin->sin_port) {
		/*
		 * Must ensure a unique port assignment.
		 */
		int enabled = osenv_intr_save_disable();

		queue_iterate(&fudp_sockets, skitmp, struct sk_impl *, chain) {
			if (skitmp == ski)
				continue;
			
			if (skitmp->sk_port == ski->sk_port) {
				if (enabled)
					osenv_intr_enable();
				return OSKIT_EADDRINUSE;
			}
		}
		ski->sk_port = sin->sin_port;
		if (enabled)
			osenv_intr_enable();
	}

	/*
	 * Is this correct?
	 */
	if (sin->sin_addr.s_addr != htonl(INADDR_ANY))
		ski->sk_ipaddr.s_addr = sin->sin_addr.s_addr;
	
	return 0;
}

/*
 * For our UDP sockets, connecting a socket is just like binding a socket.
 * We use the port/addr for both the src and dest.
 */
static OSKIT_COMDECL
sk_connect(oskit_socket_t *sk,
	   const struct oskit_sockaddr *name,
	   oskit_size_t namelen)
{
	struct sk_impl *ski = (void *)sk;
	oskit_error_t	err;

	err = sk_bind(sk, name, namelen);
	if (err)
		return err;

	ski->connected = 1;

	return 0;
}

static OSKIT_COMDECL
sk_setsockopt(oskit_socket_t *sk, oskit_u32_t level,
	      oskit_u32_t name, const void *val,
	      oskit_size_t ignored_valsize)
{
	return OSKIT_E_NOTIMPL;
}

/*
 * Send a message via UDP; no fragmentation.
 * Fill in the UDP header and pass to ip_send.
 */
static OSKIT_COMDECL
sk_sendto(oskit_socket_t *sk,
	  const void *msg, oskit_size_t msglen,
	  oskit_u32_t ignored_flags,
	  const struct oskit_sockaddr *to, oskit_size_t tolen,
	  oskit_size_t *retval)
{
	struct sk_impl *ski = (void *)sk;
	oskit_bufio_t *b;
	oskit_size_t blen;
	struct udphdr *udp;
	struct sockaddr_in *sin	= (void *)to;
	oskit_error_t err;
	unsigned short dport;
	struct in_addr ip_dst;

	assert(ski && ski->sk_count);

	if (ski->sk_port == 0 || ignored_flags)
		return OSKIT_E_INVALIDARG;

	if (!ski->connected &&
	    (!sin || sin->sin_family != OSKIT_AF_INET))
		return OSKIT_E_INVALIDARG;

	/*
	 * Allocate the bufio.
	 * It contains the whole ethernet frame.
	 * The various layers below us fill in their respective parts.
	 */
	blen = sizeof(struct ether_header) +
	       sizeof(struct ip) +
	       sizeof(struct udphdr) +
	       msglen;
	if (blen > ETH_MAX_PACKET)
		return OSKIT_EMSGSIZE;
	b = oskit_bufio_create(blen);
	if (b == NULL)
		return OSKIT_E_OUTOFMEMORY;

	/*
	 * Map it in so `udp' points to the UDP part.
	 */
	err = oskit_bufio_map(b, (void **)&udp,
			      sizeof(struct ether_header) + sizeof(struct ip),
			      sizeof(struct udphdr) + msglen);
	if (err)
		goto done;

	/*
	 * Connected sockets.
	 */
	if (ski->connected) {
		dport = ski->sk_port;
		ip_dst.s_addr = ski->sk_ipaddr.s_addr;
	}
	else {
		dport = sin->sin_port;
		ip_dst.s_addr = sin->sin_addr.s_addr;
	}

	/*
	 * Fill in the UDP header and payload.
	 */
	udp->uh_sport = ski->sk_port;		/* already in net order */
	udp->uh_dport = dport;			/* already in net order */
	udp->uh_ulen  = htons(sizeof(struct udphdr) + msglen);
	udp->uh_sum   = 0;
	memcpy((char *)udp + sizeof(struct udphdr), msg, msglen);
	oskit_bufio_unmap(b, udp,
			  sizeof(struct ether_header) + sizeof(struct ip),
			  sizeof(struct udphdr) + msglen);

	err = ip_send(&ski->sf_ipaddr, &ip_dst, &ski->sf_netmask,
		      &ski->sf_gateway, b, blen);
	if (! err)
		*retval = msglen;
 done:
	oskit_bufio_release(b);
	return err;
}

/*
 * Receive a UDP message.
 *
 * Currently all recvs are non-blocking. Must work on setsockopt.
 * If the buffer is not big enough for the message, excess bytes are dropped.
 */
static OSKIT_COMDECL
sk_recvfrom(oskit_socket_t *sk, void *buf,
	    oskit_size_t len, oskit_u32_t flags,
	    struct oskit_sockaddr *from, oskit_size_t *fromlen,
	    oskit_size_t *retval)
{
	struct sk_impl *ski = (void *)sk;
	oskit_bufio_t *b;
	struct ip *ip;
	struct udphdr *udp;
	struct sockaddr_in *sin	= (void *)from;
	oskit_error_t err;
	oskit_off_t size, usize;
	int enabled;

	assert(ski && ski->sk_count);

	if (!buf || flags || !retval)
		return OSKIT_E_INVALIDARG;

	/*
	 * Check to see if any messages at the head of the queue.
	 */
	enabled = osenv_intr_save_disable();
	if (oskit_queue_size(ski->sk_packq) == 0) {
		if (enabled)
			osenv_intr_enable();
		return OSKIT_EWOULDBLOCK;
	}
	/* Queue has something, take it. */
	oskit_queue_dequeue(ski->sk_packq, &b);
	if (enabled)
		osenv_intr_enable();
	
	err = oskit_bufio_getsize(b, &size);
	assert(err == 0);

	err = oskit_bufio_map(b, (void **)&ip,
			      sizeof(struct ether_header),
			      size - sizeof(struct ether_header));
	assert(err == 0);
	udp   = (struct udphdr *) (ip + 1);
	size -= sizeof(struct ether_header) +
		sizeof(struct ip) + sizeof(struct udphdr);
	usize = ntohs(udp->uh_ulen) - sizeof(struct udphdr);

	/*
	 * Make sure we got what we think we got
	 * Note: packet may be longer due to padding to min ethernet size.
	 */
	if (usize > size)
		panic("sk_recvfrom: packet length (%d) "
		      "too short for UDP data (%d)\n",
		      size, usize);

	if (sin) {
		sin->sin_family      = OSKIT_AF_INET;
		sin->sin_addr.s_addr = ip->ip_src.s_addr; 
		sin->sin_port        = udp->uh_sport;
	}
	memcpy(buf, (void *) (udp + 1), min(usize, len));
	*retval = usize;

	oskit_bufio_release(b);
	return 0;
}

static struct oskit_socket_ops sk_ops = {
	sk_query,
	sk_addref,
	sk_release,
	(void *)notimpl,		/* stat */
	(void *)notimpl,		/* setstat */
	(void *)notimpl,		/* pathconf */
	(void *)notimpl,		/* accept */
	sk_bind,
	sk_connect,
	(void *)notimpl,		/* shutdown */
	(void *)notimpl,		/* listen */
	(void *)notimpl,		/* getsockname */
	(void *)notimpl,		/* getpeername */
	(void *)notimpl,		/* getsockopt */
	sk_setsockopt,
	sk_sendto,
	sk_recvfrom,
	(void *)notimpl,		/* sendmsg */
	(void *)notimpl,		/* recvmsg */
};

static OSKIT_COMDECL
sk_asyncio_query(oskit_asyncio_t *f, const struct oskit_guid *iid,
	void **out_ihandle)
{
	struct sk_impl *ski = (struct sk_impl *)(f-1);

	return sk_query((oskit_socket_t *) ski, iid, out_ihandle);
}

static OSKIT_COMDECL_U
sk_asyncio_addref(oskit_asyncio_t *f)
{
	struct sk_impl *ski = (struct sk_impl *)(f-1);

	return sk_addref((oskit_socket_t *) ski);
}

static OSKIT_COMDECL_U
sk_asyncio_release(oskit_asyncio_t *f)
{
	struct sk_impl *ski = (struct sk_impl *)(f-1);

	return sk_release((oskit_socket_t *) ski);
}

/*
 * return a mask with all conditions that currently apply to that socket
 * must be called with interrupts disabled.
 */
static oskit_u32_t
get_socket_conditions(struct sk_impl *ski)
{
	oskit_u32_t	res;
	
	res = OSKIT_ASYNCIO_WRITABLE;

	if (oskit_queue_size(ski->sk_packq))
                res |= OSKIT_ASYNCIO_READABLE;

	return res;
}

/*
 * Poll for currently pending asynchronous I/O conditions.
 * If successful, returns a mask of the OSKIT_ASYNC_IO_* flags above,
 * indicating which conditions are currently present.
 */
static OSKIT_COMDECL
sk_asyncio_poll(oskit_asyncio_t *f)
{
	struct sk_impl	*ski = (struct sk_impl *)(f-1);
	int		enabled;
        oskit_u32_t     res = 0;

	enabled = osenv_intr_save_disable();
	res = get_socket_conditions(ski);
	if (enabled)
		osenv_intr_enable();
	
        return res;
}

/*
 * Add a callback object (a "listener" for async I/O events).
 * When an event of interest occurs on this I/O object
 * (i.e., when one of the three I/O conditions becomes true),
 * all registered listeners will be called.
 * Also, if successful, this method returns a mask
 * describing which of the OSKIT_ASYNC_IO_* conditions are already true,
 * which the caller must check in order to avoid missing events
 * that occur just before the listener is registered.
 */
static OSKIT_COMDECL
sk_asyncio_add_listener(oskit_asyncio_t *f, struct oskit_listener *l,
	oskit_s32_t mask)
{
	struct sk_impl	*ski = (struct sk_impl *)(f-1);
	int		enabled;
	oskit_s32_t	cond;

	if (mask & (OSKIT_ASYNCIO_WRITABLE|OSKIT_ASYNCIO_EXCEPTION))
		return OSKIT_E_INVALIDARG;
	
	enabled = osenv_intr_save_disable();
	cond = get_socket_conditions(ski);

	/* for read and exceptional conditions */
	if (mask & OSKIT_ASYNCIO_READABLE) {
		oskit_listener_mgr_add(ski->readers, l);
		ski->selecting = 1;
	}
	
	if (enabled)
		osenv_intr_enable();

        return cond;
}

/*
 * Remove a previously registered listener callback object.
 * Returns an error if the specified callback has not been registered.
 */
static OSKIT_COMDECL
sk_asyncio_remove_listener(oskit_asyncio_t *f, struct oskit_listener *l0)
{
	struct sk_impl	*ski = (struct sk_impl *)(f-1);
	int		enabled;
	oskit_error_t	err;
	
	enabled = osenv_intr_save_disable();

	err = oskit_listener_mgr_remove(ski->readers, l0);
	if (oskit_listener_mgr_count(ski->readers) == 0) {
		ski->selecting = 0;
	}

	if (enabled)
		osenv_intr_enable();

	return err;
}

static struct oskit_asyncio_ops sk_asyncioops =
{
	sk_asyncio_query,
	sk_asyncio_addref,
	sk_asyncio_release,
	sk_asyncio_poll,
	sk_asyncio_add_listener,
	sk_asyncio_remove_listener,
	(void *)notimpl,		/* readable */
};


/* Implementation of the FakeUDP socket factory. */

struct sf_impl {
	oskit_socket_factory_t	sf_ioi;
	oskit_u32_t		sf_count;
	struct in_addr		ipaddr;
	struct in_addr		netmask;
	struct in_addr		gateway;
};

static OSKIT_COMDECL
sf_query(oskit_socket_factory_t *sf, const struct oskit_guid *iid,
	 void **out_ihandle)
{
	struct sf_impl *sfi = (void *)sf;

	assert(sfi && sfi->sf_count);

        if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
            memcmp(iid, &oskit_socket_factory_iid, sizeof(*iid)) == 0) {
		*out_ihandle = &sfi->sf_ioi;
		return 0;
	}

	*out_ihandle = NULL;
	return OSKIT_E_NOINTERFACE;
}

static OSKIT_COMDECL_U
sf_addref(oskit_socket_factory_t *sf)
{
	struct sf_impl *sfi = (void *)sf;

	assert(sfi && sfi->sf_count);

	return ++sfi->sf_count;
}

/*
 * Roughly the opposite of fudp_init.
 */
static OSKIT_COMDECL_U
sf_release(oskit_socket_factory_t *sf)
{
	struct sf_impl *sfi = (void *)sf;

	assert(sfi && sfi->sf_count);

	if (--sfi->sf_count)
		return sfi->sf_count;

	free(sfi);
	return 0;
}

/*
 * Function to create a send-only UDP socket - analog to socket(2).
 */
static OSKIT_COMDECL
sf_create(oskit_socket_factory_t *sf,
	  oskit_u32_t domain, oskit_u32_t type, oskit_u32_t protocol,
	  oskit_socket_t **out_sock)
{
	struct sf_impl	*sfi = (void *)sf;
	struct sk_impl	*ski, *skitmp;
	oskit_error_t	err;
	oskit_queue_t	*packq;
	int		enabled;

	assert(sfi && sfi->sf_count);

	if (domain   != OSKIT_AF_INET ||
	    type     != OSKIT_SOCK_DGRAM ||
	    (protocol && protocol != IPPROTO_UDP))
		return OSKIT_E_INVALIDARG;

	ski = calloc(1, sizeof *ski);
	if (ski == NULL)
		return OSKIT_E_OUTOFMEMORY;

	err = oskit_bounded_com_queue_create(PACKQ_MAX, &packq);
	if (err) {
		free(ski);
		return err;
	}

	ski->sk_ioi.ops	      = &sk_ops;
	ski->sk_ioa.ops       = &sk_asyncioops;
	ski->sk_count	      = 1;
	ski->sk_packq	      = packq;
	ski->sk_ipaddr.s_addr = htonl(INADDR_ANY);
	ski->sf_ipaddr.s_addr = sfi->ipaddr.s_addr;
	ski->sf_netmask.s_addr = sfi->netmask.s_addr;
	ski->sf_gateway.s_addr = sfi->gateway.s_addr;
	ski->readers           = oskit_create_listener_mgr((oskit_iunknown_t *)
							   &ski->sk_ioa);

	enabled = osenv_intr_save_disable();

	/*
	 * Must ensure a unique port assignment.
	 */
 retry:
	ski->sk_port = htons(nextport++);
	queue_iterate(&fudp_sockets, skitmp, struct sk_impl *, chain) {
		if (skitmp->sk_port == ski->sk_port)
			goto retry;
	}
	queue_enter(&fudp_sockets, ski, struct sk_impl *, chain);
	if (enabled)
		osenv_intr_enable();
	
	*out_sock = &ski->sk_ioi;
	return 0;
}

static OSKIT_COMDECL
sf_create_pair(oskit_socket_factory_t *sf,
	  oskit_u32_t domain, oskit_u32_t type, oskit_u32_t protocol,
	  oskit_socket_t **out_sock1, oskit_socket_t **out_sock2)
{
	return OSKIT_E_NOTIMPL;
}

static struct oskit_socket_factory_ops sf_ops = {
	sf_query,
	sf_addref,
	sf_release,
	sf_create,
	sf_create_pair
};


/*
 * Get a socket factory.
 * Roughly the opposite of sf_release.
 */
oskit_error_t
udplib_init(oskit_etherdev_t *dev, char *name,
	    char *ipaddr, char *netmask, char *gateway,
	    oskit_socket_factory_t **out_factory)
{
	struct sf_impl	*sfi;
	char		haddr[OSKIT_ETHERDEV_ADDR_SIZE];
	oskit_error_t	err;
	struct in_addr	myip;

	inet_aton(ipaddr, &myip);
	if ((err = eth_initdev(dev, myip, haddr)))
		return err;

	queue_init(&fudp_sockets);

	sfi = calloc(1, sizeof *sfi);
	if (sfi == NULL)
		return OSKIT_E_OUTOFMEMORY;

	sfi->sf_ioi.ops     = &sf_ops;
	sfi->sf_count       = 1;
	inet_aton(ipaddr,  &sfi->ipaddr);
	inet_aton(netmask, &sfi->netmask);
	inet_aton(gateway, &sfi->gateway);
	farp_add(&sfi->ipaddr, haddr);
	
	*out_factory = &sfi->sf_ioi;
	return 0;
}

/*
 * Upcall from IP layer.
 */
void
socket_interrupt(oskit_bufio_t *b, oskit_size_t pkt_size)
{
	struct ether_header	*eth;
	struct ip		*ip;
	struct udphdr		*udp;
	oskit_error_t		err;
	int			enabled;
	struct sk_impl		*ski;

	err = oskit_bufio_map(b, (void **)&eth, 0, pkt_size);
	assert(err == 0);

	ip  = (struct ip *)(eth + 1);
	udp = (struct udphdr *)(ip + 1);

	/* Only want UDP datagrams */
	if (ip->ip_p != IPPROTO_UDP)
		return;

	/*
	 * Find a socket that wants this datagram
	 */
	enabled = osenv_intr_save_disable();
	queue_iterate(&fudp_sockets, ski, struct sk_impl *, chain) {
		if (ski->sk_port == udp->uh_dport) {
			oskit_queue_enqueue(ski->sk_packq, b, 0);
			if (enabled)
				osenv_intr_enable();

			/* See if someone selecting */
			if (ski->selecting)
				oskit_listener_mgr_notify(ski->readers);
		}
	}

	/* Must not have found a match. It gets dropped */
	if (enabled)
		osenv_intr_enable();
}
