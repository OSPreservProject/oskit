/*
 * Copyright (c) 1997-2001 University of Utah and the Flux Group.
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
 * This file ties together the COM interfaces to the FreeBSD networking stack.
 *
 * We create a static object (`oskit_freebsd_net_impl') to represent the stack.
 * Different COM interfaces on that object provide different groups of network
 * operations (e.g., creating sockets, or monitor/control).
 *
 * If `FLASK' is defined, we create a separate `oskit_freebsd_net_secure_impl'
 * object to provide the secure interfaces to the networking stack.

 * This file provides the `IUnknown' operations for all of the supported
 * interfaces.  It also defines the `oskit_freebsd_net_{impl,secure_impl}'
 * objects, thus tying all of the FreeBSD network COM interfaces together.
 */

#ifndef offsetof
#define offsetof(type, member) ((size_t)(&((type *)0)->member))
#endif

#include <sys/types.h>

#include "bsdnet_com.h"
#include "bsdnet_socket_factory.h"
#if FLASK
#  include "bsdnet_socket_factory_secure.h"
#endif
#include "bsdnet_mib_ip.h"
#include "bsdnet_mib_icmp.h"
#include "bsdnet_mib_tcp.h"
#include "bsdnet_mib_udp.h"


/*****************************************************************************/

/**
 ** The `IUnknown' operations for each COM interface to `bsdnet_impl_t'.
 **/

/*
 * `query'
 */
static OSKIT_COMDECL
bsdnet_impl_query(
	bsdnet_impl_t *b,
	const struct oskit_guid *iid,
	void **out_ihandle)
{
        if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
            memcmp(iid, &oskit_socket_factory_iid, sizeof(*iid)) == 0) {
		*out_ihandle = &b->sf;
		return 0;
	}
	
        if (memcmp(iid, &oskit_mib_ip_iid, sizeof(*iid)) == 0) {
                *out_ihandle = &b->ip_mib;
                return 0;
	}
	
        if (memcmp(iid, &oskit_mib_icmp_iid, sizeof(*iid)) == 0) {
                *out_ihandle = &b->icmp_mib;
                return 0;
	}
	
        if (memcmp(iid, &oskit_mib_tcp_iid, sizeof(*iid)) == 0) {
                *out_ihandle = &b->tcp_mib;
                return 0;
	}
	
        if (memcmp(iid, &oskit_mib_udp_iid, sizeof(*iid)) == 0) {
                *out_ihandle = &b->udp_mib;
                return 0;
	}
	
	*out_ihandle = 0;
	return OSKIT_E_NOINTERFACE;
}

static OSKIT_COMDECL
bsdnet_socket_factory_query(
	oskit_socket_factory_t *intf,
	const struct oskit_guid *iid,
	void **out_ihandle)
{
	bsdnet_impl_t *impl
		= ((bsdnet_impl_t *)
		   (((char *) intf) - offsetof(bsdnet_impl_t, sf)));
	return bsdnet_impl_query(impl, iid, out_ihandle);
}

static OSKIT_COMDECL
bsdnet_mib_ip_query(
	oskit_mib_ip_t *intf,
	const struct oskit_guid *iid,
	void **out_ihandle)
{
	bsdnet_impl_t *impl
		= ((bsdnet_impl_t *)
		   (((char *) intf) - offsetof(bsdnet_impl_t, ip_mib)));
	return bsdnet_impl_query(impl, iid, out_ihandle);
}

static OSKIT_COMDECL
bsdnet_mib_icmp_query(
	oskit_mib_icmp_t *intf,
	const struct oskit_guid *iid,
	void **out_ihandle)
{
	bsdnet_impl_t *impl
		= ((bsdnet_impl_t *)
		   (((char *) intf) - offsetof(bsdnet_impl_t, icmp_mib)));
	return bsdnet_impl_query(impl, iid, out_ihandle);
}

static OSKIT_COMDECL
bsdnet_mib_tcp_query(
	oskit_mib_tcp_t *intf,
	const struct oskit_guid *iid,
	void **out_ihandle)
{
	bsdnet_impl_t *impl
		= ((bsdnet_impl_t *)
		   (((char *) intf) - offsetof(bsdnet_impl_t, tcp_mib)));
	return bsdnet_impl_query(impl, iid, out_ihandle);
}

static OSKIT_COMDECL
bsdnet_mib_udp_query(
	oskit_mib_udp_t *intf,
	const struct oskit_guid *iid,
	void **out_ihandle)
{
	bsdnet_impl_t *impl
		= ((bsdnet_impl_t *)
		   (((char *) intf) - offsetof(bsdnet_impl_t, udp_mib)));
	return bsdnet_impl_query(impl, iid, out_ihandle);
}

/*
 * `addref'
 */
static OSKIT_COMDECL_U
bsdnet_impl_addref(bsdnet_impl_t *impl)
{
	return 1;
}

static OSKIT_COMDECL_U
bsdnet_socket_factory_addref(oskit_socket_factory_t *intf)
{
	bsdnet_impl_t *impl
		= ((bsdnet_impl_t *)
		   (((char *) intf) - offsetof(bsdnet_impl_t, sf)));
	return bsdnet_impl_addref(impl);
}

static OSKIT_COMDECL_U
bsdnet_mib_ip_addref(oskit_mib_ip_t *intf)
{
	bsdnet_impl_t *impl
		= ((bsdnet_impl_t *)
		   (((char *) intf) - offsetof(bsdnet_impl_t, ip_mib)));
	return bsdnet_impl_addref(impl);
}

static OSKIT_COMDECL_U
bsdnet_mib_icmp_addref(oskit_mib_icmp_t *intf)
{
	bsdnet_impl_t *impl
		= ((bsdnet_impl_t *)
		   (((char *) intf) - offsetof(bsdnet_impl_t, icmp_mib)));
	return bsdnet_impl_addref(impl);
}

static OSKIT_COMDECL_U
bsdnet_mib_tcp_addref(oskit_mib_tcp_t *intf)
{
	bsdnet_impl_t *impl
		= ((bsdnet_impl_t *)
		   (((char *) intf) - offsetof(bsdnet_impl_t, tcp_mib)));
	return bsdnet_impl_addref(impl);
}

static OSKIT_COMDECL_U
bsdnet_mib_udp_addref(oskit_mib_udp_t *intf)
{
	bsdnet_impl_t *impl
		= ((bsdnet_impl_t *)
		   (((char *) intf) - offsetof(bsdnet_impl_t, udp_mib)));
	return bsdnet_impl_addref(impl);
}

/*
 * `release'
 */
static OSKIT_COMDECL_U
bsdnet_impl_release(bsdnet_impl_t *impl)
{
	return 1;
}

static OSKIT_COMDECL_U
bsdnet_socket_factory_release(oskit_socket_factory_t *intf)
{
	bsdnet_impl_t *impl
		= ((bsdnet_impl_t *)
		   (((char *) intf) - offsetof(bsdnet_impl_t, sf)));
	return bsdnet_impl_release(impl);
}

static OSKIT_COMDECL_U
bsdnet_mib_ip_release(oskit_mib_ip_t *intf)
{
	bsdnet_impl_t *impl
		= ((bsdnet_impl_t *)
		   (((char *) intf) - offsetof(bsdnet_impl_t, ip_mib)));
	return bsdnet_impl_release(impl);
}

static OSKIT_COMDECL_U
bsdnet_mib_icmp_release(oskit_mib_icmp_t *intf)
{
	bsdnet_impl_t *impl
		= ((bsdnet_impl_t *)
		   (((char *) intf) - offsetof(bsdnet_impl_t, icmp_mib)));
	return bsdnet_impl_release(impl);
}

static OSKIT_COMDECL_U
bsdnet_mib_tcp_release(oskit_mib_tcp_t *intf)
{
	bsdnet_impl_t *impl
		= ((bsdnet_impl_t *)
		   (((char *) intf) - offsetof(bsdnet_impl_t, tcp_mib)));
	return bsdnet_impl_release(impl);
}

static OSKIT_COMDECL_U
bsdnet_mib_udp_release(oskit_mib_udp_t *intf)
{
	bsdnet_impl_t *impl
		= ((bsdnet_impl_t *)
		   (((char *) intf) - offsetof(bsdnet_impl_t, udp_mib)));
	return bsdnet_impl_release(impl);
}


/*****************************************************************************/

#if FLASK

/**
 ** The `IUnknown' operations for each COM interface to `bsdnet_impl_secure_t'.
 **/

/*
 * `query'
 */
static OSKIT_COMDECL
bsdnet_socket_factory_secure_query(
	oskit_socket_factory_secure_t *b,
	const struct oskit_guid *iid,
	void **out_ihandle)
{
	if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
            memcmp(iid, &oskit_socket_factory_secure_iid, sizeof(*iid)) == 0) {
		*out_ihandle = b;
		return 0;
	}
	
	*out_ihandle = 0;
	return OSKIT_E_NOINTERFACE;
}

/*
 * `addref'
 */
static OSKIT_COMDECL_U
bsdnet_socket_factory_secure_addref(oskit_socket_factory_secure_t *b)
{
	return 1;
}

/*
 * `release'
 */
static OSKIT_COMDECL_U
bsdnet_socket_factory_secure_release(oskit_socket_factory_secure_t *b)
{
	return 1;
}

#endif /* FLASK */


/*****************************************************************************/

/**
 ** Create the object that provides all of the COM interfaces to the FreeBSD
 ** networking stack (except for the `oskit_socket_factory_secure' interface,
 ** which is provided by a different object).
 **/

static struct oskit_socket_factory_ops socket_factory_ops = {
	bsdnet_socket_factory_query,
	bsdnet_socket_factory_addref,
	bsdnet_socket_factory_release,
	
	bsdnet_socket_factory_create,
	bsdnet_socket_factory_create_pair
};

static struct oskit_mib_ip_ops mib_ip_ops = {
	bsdnet_mib_ip_query,
	bsdnet_mib_ip_addref,
	bsdnet_mib_ip_release,
	
	OSKIT_MIB_SLOTSET_IMPLS_RO(
		bsdnet_mib_ip_get_ipstats),
	OSKIT_MIB_SLOT_IMPLS_RW(
		bsdnet_mib_ip_get_ip_forwarding,
		bsdnet_mib_ip_set_ip_forwarding),
	OSKIT_MIB_SLOT_IMPLS_RW(
		bsdnet_mib_ip_get_ip_default_ttl,
		bsdnet_mib_ip_set_ip_default_ttl),
	OSKIT_MIB_SLOTSET_TABLE_IMPLS_RO(
		bsdnet_mib_ip_get_ipaddr_table),
	OSKIT_MIB_SLOTSET_TABLE_IMPLS_RO(
		bsdnet_mib_ip_get_ipnettomedia_table),
	OSKIT_MIB_SLOTSET_TABLE_IMPLS_RW(
		bsdnet_mib_ip_get_ipforward_table,
		bsdnet_mib_ip_set_ipforward_table)
};

static struct oskit_mib_icmp_ops mib_icmp_ops = {
	bsdnet_mib_icmp_query,
	bsdnet_mib_icmp_addref,
	bsdnet_mib_icmp_release,
	
	OSKIT_MIB_SLOTSET_IMPLS_RO(
		bsdnet_mib_icmp_get_icmpstats)
};

static struct oskit_mib_tcp_ops mib_tcp_ops = {
	bsdnet_mib_tcp_query,
	bsdnet_mib_tcp_addref,
	bsdnet_mib_tcp_release,
	
	OSKIT_MIB_SLOTSET_IMPLS_RO(
		bsdnet_mib_tcp_get_tcpstats),
	OSKIT_MIB_SLOTSET_TABLE_IMPLS_RW(
		bsdnet_mib_tcp_get_tcpconn_table,
		bsdnet_mib_tcp_set_tcpconn_table)
};

static struct oskit_mib_udp_ops mib_udp_ops = {
	bsdnet_mib_udp_query,
	bsdnet_mib_udp_addref,
	bsdnet_mib_udp_release,
	
	OSKIT_MIB_SLOTSET_IMPLS_RO(
		bsdnet_mib_udp_get_udpstats),
	OSKIT_MIB_SLOTSET_TABLE_IMPLS_RO(
		bsdnet_mib_udp_get_udpentry_table)
};

/*
 * Finally, our object!
 */
bsdnet_impl_t oskit_freebsd_net_impl = {
	{ &socket_factory_ops },
	{ &mib_ip_ops },
	{ &mib_icmp_ops },
	{ &mib_tcp_ops },
	{ &mib_udp_ops }
};


/*****************************************************************************/

#if FLASK

/**
 ** Create the object that provides the `oskit_socket_factory_secure_t'
 ** interface to the FreeBSD networking stack.
 **/

static struct oskit_socket_factory_secure_ops socket_factory_secure_ops = {
	bsdnet_socket_factory_secure_query,
	bsdnet_socket_factory_secure_addref,
	bsdnet_socket_factory_secure_release,
	
	bsdnet_socket_factory_secure_create,
	bsdnet_socket_factory_secure_create_pair
};

/*
 * Finally, our object!
 */
bsdnet_impl_secure_t oskit_freebsd_net_secure_impl = {
	{ &socket_factory_secure_ops }
};

#endif /* FLASK */

/*****************************************************************************/

/* End of file. */

