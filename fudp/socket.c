/*
 * Copyright (c) 1998 The University of Utah and the Flux Group.
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
 * A send-only UDP interface without fragmentation.
 */

#include <oskit/net/socket.h>
#include <oskit/io/bufio.h>
#include <oskit/io/netio.h>
#include <oskit/c/netinet/in.h>
#include <oskit/c/netinet/udp.h>
#include <oskit/c/netinet/ip.h>
#include <oskit/c/netinet/udp.h>
#include <oskit/c/arpa/inet.h>
#include <oskit/c/string.h>
#include <oskit/c/stdlib.h>
#include <oskit/c/assert.h>
#include <oskit/net/ether.h>

#include <oskit/fudp.h>
#include "fudp.h"

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
	oskit_u32_t	sk_count;

	oskit_u16_t	sk_port;	/* net order */
	struct in_addr	sk_ipaddr;	/* net order */
	oskit_netio_t	*sk_netio;
};

static OSKIT_COMDECL
sk_query(oskit_socket_t *sk, const struct oskit_guid *iid, void **out_ihandle)
{
	struct sk_impl *ski = (void *)sk;

	assert(ski && ski->sk_count);

        if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
            memcmp(iid, &oskit_socket_iid, sizeof(*iid)) == 0) {
		*out_ihandle = &ski->sk_ioi;
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

	assert(ski && ski->sk_count);

	if (--ski->sk_count)
		return ski->sk_count;

	if (ski->sk_netio)
		oskit_netio_release(ski->sk_netio);
	free(ski);
	return 0;
}

/*
 * Assign the port and ipaddr for the socket to use.
 *
 * XXX In the future we could pick a port in `create' and then
 * this routine will be only useful if you need a specific name
 * for a socket.
 * Maybe we could define a new family type and have the sockaddr
 * contain the netio?
 */
static OSKIT_COMDECL
sk_bind(oskit_socket_t *sk,
	const struct oskit_sockaddr *name,
	oskit_size_t namelen)
{
	struct sk_impl *ski = (void *)sk;
	struct sockaddr_in *sin	= (void *)name;

	assert(ski && ski->sk_count);

	if (sin->sin_family != OSKIT_AF_INET)
		return OSKIT_E_INVALIDARG;

	ski->sk_ipaddr.s_addr = sin->sin_addr.s_addr;
	ski->sk_port = sin->sin_port;
	return 0;
}

static OSKIT_COMDECL
sk_setsockopt(oskit_socket_t *sk, oskit_u32_t level,
	      oskit_u32_t name, const void *val,
	      oskit_size_t ignored_valsize)
{
	struct sk_impl *ski = (void *)sk;

	assert(ski && ski->sk_count);

	if (level != OSKIT_SOL_SOCKET ||
	    name != OSKIT_SO_NETIO ||
	    val == NULL)
		return OSKIT_E_INVALIDARG;

	ski->sk_netio = (oskit_netio_t *)val;
	oskit_netio_addref(ski->sk_netio);
	return 0;
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

	assert(ski && ski->sk_count);

	if (ski->sk_port == 0 ||
	    ski->sk_ipaddr.s_addr == htonl(INADDR_ANY) ||
	    ski->sk_netio == NULL ||
	    sin->sin_family != OSKIT_AF_INET ||
	    ignored_flags)
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
	 * Fill in the UDP header and payload.
	 */
	udp->uh_sport = ski->sk_port;		/* already in net order */
	udp->uh_dport = sin->sin_port;		/* already in net order */
	udp->uh_ulen  = htons(sizeof(struct udphdr) + msglen);
	udp->uh_sum   = 0;
	memcpy((char *)udp + sizeof(struct udphdr), msg, msglen);
	oskit_bufio_unmap(b, udp,
			  sizeof(struct ether_header) + sizeof(struct ip),
			  sizeof(struct udphdr) + msglen);

	err = ip_send(ski->sk_netio, &ski->sk_ipaddr, &sin->sin_addr, b, blen);
	if (! err)
		*retval = msglen;
 done:
	oskit_bufio_release(b);
	return err;
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
	(void *)notimpl,		/* connect */
	(void *)notimpl,		/* shutdown */
	(void *)notimpl,		/* listen */
	(void *)notimpl,		/* getsockname */
	(void *)notimpl,		/* getpeername */
	(void *)notimpl,		/* getsockopt */
	sk_setsockopt,
	sk_sendto,
	(void *)notimpl,		/* recvfrom */
	(void *)notimpl,		/* sendmsg */
	(void *)notimpl,		/* recvmsg */
};


/* Implementation of the FakeUDP socket factory. */

struct sf_impl {
	oskit_socket_factory_t sf_ioi;
	oskit_u32_t	sf_count;
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
 * The socket created is not yet ready for use, it needs a
 * bind() call to give it a port & ipaddr and a setsockopt() to give it
 * the netio to use.
 */
static OSKIT_COMDECL
sf_create(oskit_socket_factory_t *sf,
	  oskit_u32_t domain, oskit_u32_t type, oskit_u32_t protocol,
	  oskit_socket_t **out_sock)
{
	struct sf_impl *sfi = (void *)sf;
	struct sk_impl *ski;

	assert(sfi && sfi->sf_count);

	if (domain   != OSKIT_AF_INET ||
	    type     != OSKIT_SOCK_DGRAM ||
	    protocol != IPPROTO_UDP)
		return OSKIT_E_INVALIDARG;

	ski = malloc(sizeof *ski);
	if (ski == NULL)
		return OSKIT_E_OUTOFMEMORY;

	ski->sk_ioi.ops	      = &sk_ops;
	ski->sk_count	      = 1;
	ski->sk_port	      = 0;
	ski->sk_ipaddr.s_addr = htonl(INADDR_ANY);
	ski->sk_netio	      = NULL;

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
fudp_init(oskit_socket_factory_t **out_factory)
{
	struct sf_impl *sfi;

	sfi = malloc(sizeof *sfi);
	if (sfi == NULL)
		return OSKIT_E_OUTOFMEMORY;

	sfi->sf_ioi.ops        = &sf_ops;
	sfi->sf_count          = 1;

	*out_factory = &sfi->sf_ioi;
	return 0;
}
