/*
 * Copyright (c) 2001 University of Utah and the Flux Group.
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
 * The FreeBSD networking stack's implementation of the `oskit_mib_ip' COM
 * interface.
 */

#include <oskit/mib.h>
#include <oskit/mib/ip.h>
#include <oskit/dev/freebsd.h>	/* For `oskit_freebsd_xlate_errno'. */

#include "bsdnet_mib_ip.h"

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/sysctl.h>
#include <sys/protosw.h>	/* For `PR_SLOWHZ'. */
#include <net/if.h>
#include <net/if_dl.h>
#include <net/route.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>	/* For <netinet/ip.h>. */
#include <netinet/ip.h>		/* For `IPFRAGTTL'. */
#include <netinet/ip_var.h>

/*
 * The following `SYSCTL' data items are defined in the OSKit file
 * `freebsd/3.x/src/sys/netinet/ip_input.c'.
 */
extern int		ipforwarding;
/* extern int		ipsendredirects; */
extern int		ip_defttl;
/* extern int		ip_dosourceroute; */
/* extern int		ip_acceptsourceroute; */
/* ...			ipintrq.ifq_maxlen; */
/* ...			ipintrq.ifq_drops; */
extern struct ipstat	ipstat;
/* extern ?		ip_mtu; */

/*
 * The following `SYSCTL' data items are defined in the OSKit file
 * `freebsd/3.x/src/sys/net/rtsock.c'.
 */
extern int		sysctl_rtsock SYSCTL_HANDLER_ARGS;

/*
 * Types and macros for debugging our `sysctl'-style callback functions.
 */
#ifdef DEBUG
typedef enum { INIT, SEEN_IFINFO } env_state;
#  define DEBUG_CHECK_STATE(expr)	assert((expr))
#  define DEBUG_SET_STATE(expr)		(expr)
#else
#  define DEBUG_CHECK_STATE(expr)	/* Do nothing. */
#  define DEBUG_SET_STATE(expr)		/* Do nothing. */
#endif /* DEBUG */


/*****************************************************************************/

OSKIT_COMDECL
bsdnet_mib_ip_get_ipstats(
	oskit_mib_ip_t *			intf,
	/* OUT */ oskit_mib_slotset_ipstats_t *	ss)
{
	oskit_u32_t ip_in_hdr_errors
		= (ipstat.ips_badsum + ipstat.ips_tooshort
		   + ipstat.ips_toosmall + ipstat.ips_badhlen
		   + ipstat.ips_badlen);
	oskit_u32_t ip_reasm_fails
		= (ipstat.ips_fragdropped + ipstat.ips_fragtimeout);
	
	/*****/
	
	oskit_mib_bool_set(&(ss->ip_forwarding), ipforwarding);
	oskit_mib_u8_set(&(ss->ip_default_ttl), ip_defttl);
	
	oskit_mib_u32_set(&(ss->ip_in_receives), ipstat.ips_total);
	oskit_mib_u32_set(&(ss->ip_in_hdr_errors), ip_in_hdr_errors);
	oskit_mib_u32_set(&(ss->ip_in_addr_errors), ipstat.ips_cantforward);
	oskit_mib_u32_set(&(ss->ip_forw_datagrams), ipstat.ips_forward);
	oskit_mib_u32_set(&(ss->ip_in_unknown_protos), ipstat.ips_noproto);
	oskit_mib_u32_set(&(ss->ip_in_discards), ipstat.ips_fragdropped);
	oskit_mib_u32_set(&(ss->ip_in_delivers), ipstat.ips_delivered);
	
	oskit_mib_u32_set(&(ss->ip_out_requests), ipstat.ips_localout);
	oskit_mib_u32_set(&(ss->ip_out_discards), ipstat.ips_odropped);
	oskit_mib_u32_set(&(ss->ip_out_no_routes), ipstat.ips_noroute);
	
	/* Warning: `ip_reasm_timeout' is *not* `ipstat.ips_fragtimeout'. */
	/* `ip_reasm_timeout' is the timeout value, not a packet count.   */
	oskit_mib_s32_set(&(ss->ip_reasm_timeout), (IPFRAGTTL / PR_SLOWHZ));
	oskit_mib_u32_set(&(ss->ip_reasm_reqds), ipstat.ips_fragments);
	oskit_mib_u32_set(&(ss->ip_reasm_oks), ipstat.ips_reassembled);
	oskit_mib_u32_set(&(ss->ip_reasm_fails), ip_reasm_fails);
	
	oskit_mib_u32_set(&(ss->ip_frags_ok), ipstat.ips_fragmented);
	oskit_mib_u32_set(&(ss->ip_frags_fails), ipstat.ips_cantfrag);
	oskit_mib_u32_set(&(ss->ip_frags_creates), ipstat.ips_ofragments);
	
	/* Warning: `ip_routing_discards' is *not* `ipstat.ips_noroute. */
	/* `ip_routing_discards' is count of discarded routing entries. */
	/* But the FreeBSD stack doesn't keep such a counter (?).       */
	oskit_mib_u32_clear(&(ss->ip_routing_discards));
	
	/* XXX --- Need to find impl.  Is this easy to figure out? */
	oskit_mib_u32_clear(&(ss->ip_forward_number));
	
	return OSKIT_S_OK;
}

OSKIT_COMDECL
bsdnet_mib_ip_get_ip_forwarding(
	oskit_mib_ip_t *		intf,
	/* OUT */ oskit_mib_bool_t *	val)
{
	oskit_mib_bool_set(val, ipforwarding);
	return OSKIT_S_OK;
}

OSKIT_COMDECL
bsdnet_mib_ip_set_ip_forwarding(
	oskit_mib_ip_t *		intf,
	oskit_bool_t *			val)
{
	ipforwarding = *val;
	return OSKIT_S_OK;
}

OSKIT_COMDECL
bsdnet_mib_ip_get_ip_default_ttl(
	oskit_mib_ip_t *		intf,
	/* OUT */ oskit_mib_u8_t *	val)
{
	oskit_mib_u8_set(val, ip_defttl);
	return OSKIT_S_OK;
}

OSKIT_COMDECL
bsdnet_mib_ip_set_ip_default_ttl(
	oskit_mib_ip_t *		intf,
	oskit_u8_t *			val)
{
	ip_defttl = *val;
	return OSKIT_S_OK;
}


/*****************************************************************************/

/*
 * This is the type of the environment that is passed to the `get_one_ipaddr'
 * callback function.
 */
typedef struct get_ipaddr_env get_ipaddr_env_t;
struct get_ipaddr_env {
	/*
	 * The `sysctl' request data.  This must be the first struct member.
	 * The callback function receives a `struct sysctl_req *', and then
	 * casts it to be a pointer to this environment structure.
	 */
	struct sysctl_req	req;
	
	/*
	 * The arguments to `bsdnet_mib_udp_get_ipaddr_table', which sets up
	 * the environment for the callback.
	 */
	struct {
		oskit_u32_t					start_row;
		oskit_u32_t					want_rows;
		oskit_mib_slotset_ipaddr_t *			matching;
		/* OUT */ oskit_mib_slotset_ipaddr_t *		table;
		/* OUT */ oskit_u32_t *				out_rows;
		/* OUT */ oskit_bool_t *			more_rows;
	} args;
	
	/*
	 * State that must be kept between invocations of the callback.
	 */
	int			ifinfo_flags;	/* Current intf. flags. */
#ifdef DEBUG
	env_state		state;
#endif
};

/*
 * The following function is copied almost directly from FreeBSD's `ifconfig'
 * program.  In FreeBSD, see `/usr/src/sbin/ifconfig/ifconfig.c'.
 *
 * Expand the compacted form of addresses as returned via the configuration
 * read via sysctl().
 */

#define ROUNDUP(a) \
	((a) > 0 ? (1 + (((a) - 1) | (sizeof(long) - 1))) : sizeof(long))
#define ADVANCE(x, n) (x += ROUNDUP((n)->sa_len))

static void
rt_xaddrs(char *cp, char *cplim, struct rt_addrinfo *rtinfo)
{
	struct sockaddr *sa;
	int i;
	
	memset(rtinfo->rti_info, 0, sizeof(rtinfo->rti_info));
	for (i = 0; (i < RTAX_MAX) && (cp < cplim); i++) {
		if ((rtinfo->rti_addrs & (1 << i)) == 0)
			continue;
		rtinfo->rti_info[i] = sa = (struct sockaddr *) cp;
		ADVANCE(cp, sa);
	}
}

/*
 * Determine if the information about the given IP address entry
 * (`ip_ad_ent_*') matches the given filter.
 */
static inline oskit_bool_t
ipaddr_match(
	oskit_mib_in_addr_t		ip_ad_ent_addr,
	oskit_u32_t			ip_ad_ent_if_index,
	oskit_mib_in_addr_t		ip_ad_ent_net_mask,
	oskit_u32_t			ip_ad_ent_bcast_addr,
	oskit_u16_t			ip_ad_ent_reasm_max_size,
	oskit_mib_slotset_ipaddr_t *	filter)
{
	if (!filter)
		return 1;
	
#define MATCH_SLOT(slot_type, slot_name)				\
	if (oskit_mib_##slot_type##_test(&(filter->slot_name))		\
	    && (slot_name						\
		!= oskit_mib_##slot_type##_get(&(filter->slot_name))))	\
		return 0
	
	MATCH_SLOT(mib_in_addr,	ip_ad_ent_addr);
	MATCH_SLOT(u32,		ip_ad_ent_if_index);
	MATCH_SLOT(mib_in_addr,	ip_ad_ent_net_mask);
	MATCH_SLOT(u32,		ip_ad_ent_bcast_addr);
	MATCH_SLOT(u16,		ip_ad_ent_reasm_max_size);
#undef MATCH_SLOT
	
	return 1;
}

/*
 * This is the ``callback function'' that is invoked by `sysctl_rtsock' in
 * order to copy data out.  The data that is given to us is as follows:
 *
 *   + First, we are called with a buffer containing a `struct if_msghdr', with
 *     the type member set to `RTM_IFINFO'.  This signals the start of a group
 *     of callbacks about a particular interface.  The `if_msghdr' is followed
 *     by zero of more `struct sockaddr_dl's (in the same buffer).
 *
 *   + Subsequent callbacks will receive a `struct ifa_msghdr' (with its type
 *     member set to `RTM_NEWADDR') followed immediatley by zero or more
 *     `struct sockaddr_in's.  This describes a particular group of (IP)
 *     addresses for an interface.
 *
 * These callbacks are performed one for each interface.  I.e., for each
 * interface, we will receive a `RTM_IFINFO' callback followed by zero or more
 * `RTM_NEWADDR' callbacks.
 *
 * See the function `sysctl_rtsock' for further details.  Some of the code
 * below is modeled after code in the FreeBSD `ifconfig' utility; in FreeBSD,
 * see the file `/usr/src/sbin/ifconfig/ifconfig.c'.
 */
static int
get_one_ipaddr(
	struct sysctl_req *	req,
	const void *		buf,
	size_t			buf_size)
{
	get_ipaddr_env_t *	env = (get_ipaddr_env_t *) req;
	struct ifa_msghdr *	ifam = (struct ifa_msghdr *) buf;
	
	struct rt_addrinfo	rtinfo;
	struct sockaddr_in *	sin;
	
	oskit_mib_in_addr_t	addr;
	oskit_u32_t		if_index;
	oskit_mib_in_addr_t	net_mask;
	oskit_u32_t		bcast_addr;
	oskit_u16_t		reasm_max_size;
	
	/*********************************************************************/
	
	/*
	 * Mainly, we care about `RTM_NEWADDR' messages.  Each of these
	 * messages contains a `struct ifa_msghdr' followed by a set of socket
	 * addresses (`struct sockaddr_in's).
	 *
	 * We need `RTM_IFINFO' messages only to set our `env->ifinfo_flags'.
	 * One of these messages contains a `struct if_msghdr' (followed by a
	 * set of `struct sockaddr_dl's, which we don't care about here).
	 */
	if (ifam->ifam_type == RTM_IFINFO) {
		env->ifinfo_flags = ((struct if_msghdr *) buf)->ifm_flags;
		DEBUG_SET_STATE((env->state = SEEN_IFINFO));
		goto done;
	}
	
	if (ifam->ifam_type != RTM_NEWADDR) {
		assert(!"Received an unknown type of message.");
		goto done;
	}
	
	DEBUG_CHECK_STATE((env->state == SEEN_IFINFO));
	
	/*********************************************************************/
	
	/*
	 * Figure out what the socket addresses mean; i.e., assign them to the
	 * appropriate elements of the `rtinfo.rti_info' array.
	 */
	rtinfo.rti_addrs = ifam->ifam_addrs;
	rt_xaddrs(((char *) (ifam + 1)),
		  ((char *) ifam) + ifam->ifam_msglen,
		  &rtinfo);
	
	/*
	 * From `rtinfo', decode the fields of our table row.
	 */
	
	sin = (struct sockaddr_in *) rtinfo.rti_info[RTAX_IFA];
	if (!sin)
		/* No interface address?! */
		goto done;
	addr = sin->sin_addr.s_addr;
	
	if_index = ifam->ifam_index;
	
	sin = (struct sockaddr_in *) rtinfo.rti_info[RTAX_NETMASK];
	if (!sin)
		/* No network mask?! */
		goto done;
	net_mask = sin->sin_addr.s_addr;
	
	/* XXX --- Default `bcast_addr' to one. */
	bcast_addr = 1;
	/*
	 * For point-to-point links, `rti_info[RTAX_BRD]' holds the destination
	 * address.  So, one must check for the `IFF_BROADCAST' flag.
	 */
	if (env->ifinfo_flags & IFF_BROADCAST) {
		sin = (struct sockaddr_in *) rtinfo.rti_info[RTAX_BRD];
		if (sin)
			bcast_addr = (ntohl(sin->sin_addr.s_addr) & 1);
	}
	
	/*
	 * I'm pretty sure that the `reasm_max_size' is `IP_MAXPACKET' for all
	 * interfaces.
	 */
	reasm_max_size = IP_MAXPACKET;
	
	/*********************************************************************/
	
	/*
	 * Process the entry.
	 */
	if (ipaddr_match(addr, if_index, net_mask, bcast_addr, reasm_max_size,
			 env->args.matching)) {
		if (env->args.start_row > 0) {
			/* Skip this row. */
			env->args.start_row--;
		} else if (env->args.want_rows > 0) {
			/* Accumulate this row. */
#define COPY_OUT(slot_type, slot_name)				\
	oskit_mib_##slot_type##_set(				\
		&(env->args.table->ip_ad_ent_##slot_name),	\
		slot_name)
			COPY_OUT(mib_in_addr, addr);
			COPY_OUT(u32, if_index);
			COPY_OUT(mib_in_addr, net_mask);
			COPY_OUT(u32, bcast_addr);
			COPY_OUT(u16, reasm_max_size);
#undef COPY_OUT
			(*env->args.out_rows)++;
			env->args.table++;
			env->args.want_rows--;
			
		} else {
			/* Indicate that the table filled up. */
			*env->args.more_rows = 1;
		}
	}
	
 done:
	/* Indicate to the `sysctl_rtsock' function that no error occurred. */
	return 0;
}

/*
 * The implementation of the `get_ipaddr_table' operation.
 */
OSKIT_COMDECL
bsdnet_mib_ip_get_ipaddr_table(
	oskit_mib_ip_t *				intf,
	oskit_u32_t					start_row,
	oskit_u32_t					want_rows,
	oskit_mib_slotset_ipaddr_t *			matching,
	/* OUT */ oskit_mib_slotset_ipaddr_t *		table,
	/* OUT */ oskit_u32_t *				out_rows,
	/* OUT */ oskit_bool_t *			more_rows)
{
	int			rt_iflist[] = { 0, AF_INET, NET_RT_IFLIST, 0 };
	get_ipaddr_env_t	env;
	int			rc;
	
	/*
	 * This implementation works by calling FreeBSD's `sysctl' function for
	 * scanning the list of interface entries.  We have to do a little work
	 * to call that function, since it expects a `sysctl'-style interface,
	 * but this actually works to our advantage.  Part of the interface is
	 * that the caller provides a function to ``copy out'' data.  Our
	 * copy-out function fills out the table.  Clever, eh?
	 */
	
	/*
	 * Set up the environment for the callback.
	 *
	 * Note that `env.req.oldptr' must be non-null.  This is part of the
	 * interface to `systcl'-style functions and tells the function that
	 * we're reading the current state.  Similarly, `env.req.newptr' must
	 * be null to tell the function that we're not writing new data.
	 */
	env.req.p		= 0;
	env.req.lock		= 0;
	env.req.oldptr		= &env;		/* Must be non-null. */
	env.req.oldlen		= sizeof(env);
	env.req.oldidx		= 0;
	env.req.oldfunc		= get_one_ipaddr;
	env.req.newptr		= 0;		/* Must be null. */
	env.req.newlen		= 0;
	env.req.newidx		= 0;
	env.req.newfunc		= 0;
	env.args.start_row	= start_row;
	env.args.want_rows	= want_rows;
	env.args.matching	= matching;
	env.args.table		= table;
	env.args.out_rows	= out_rows;
	*(env.args.out_rows)	= 0;
	env.args.more_rows	= more_rows;
	*(env.args.more_rows)	= 0;
	env.ifinfo_flags	= 0;
	DEBUG_SET_STATE((env.state = INIT));
	
	/*
	 * The first arg to `sysctl_rtsock' is unused.  The second names the
	 * portion of the FreeBSD MIB that we want, and the third is the number
	 * of elements in the second arg.
	 */
	rc = sysctl_rtsock(0,
			   rt_iflist,
			   (sizeof(rt_iflist) / sizeof(int)),
			   &(env.req));
	
	/*
	 * Nothing to say here about the debug state.  If there were no
	 * interfaces, we could still be in the INIT state.
	 */
	
	return (rc ? oskit_freebsd_xlate_errno(rc) : OSKIT_S_OK);
}


/*****************************************************************************/

/*
 * This is the type of the environment that is passed to the
 * `get_one_ipnettomedia' callback function.
 */
typedef struct get_ipnettomedia_env get_ipnettomedia_env_t;
struct get_ipnettomedia_env {
	/*
	 * The `sysctl' request data.  This must be the first struct member.
	 * The callback function receives a `struct sysctl_req *', and then
	 * casts it to be a pointer to this environment structure.
	 */
	struct sysctl_req	req;
	
	/*
	 * The arguments to `bsdnet_mib_udp_get_ipnettomedia_table', which sets
	 * up the environment for the callback.
	 */
	struct {
		oskit_u32_t					start_row;
		oskit_u32_t					want_rows;
		oskit_mib_slotset_ipnettomedia_t *		matching;
		/* OUT */ oskit_mib_slotset_ipnettomedia_t *	table;
		/* OUT */ oskit_u32_t *				out_rows;
		/* OUT */ oskit_bool_t *			more_rows;
	} args;
	
	/*
	 * State that must be kept between invocations of the callback.
	 */
	oskit_mib_string_t	phys_address;	/* Current intf. addr. */
#define PHYS_ADDRESS_SIZE (64)
	char			phys_address_data[PHYS_ADDRESS_SIZE];
#ifdef DEBUG
	env_state		state;
#endif
};

/*
 * Extract the physcal address data from `sdl' and copy it into `env'.
 */
static void
extract_phys_address(
	struct sockaddr_dl *		sdl,
	get_ipnettomedia_env_t *	env)
{
	if (sdl->sdl_alen >= PHYS_ADDRESS_SIZE) {
		env->phys_address.str = 0;
		env->phys_address.len = 0;
		env->phys_address.max = 0;
		return;
	}
	memcpy(env->phys_address_data, LLADDR(sdl), sdl->sdl_alen);
	env->phys_address.str = env->phys_address_data;
	env->phys_address.len = sdl->sdl_alen;
	env->phys_address.max = PHYS_ADDRESS_SIZE;
}

/*
 * Determine if the information about the given IP net-to-media entry
 * (`ip_net_to_media_*') matches the given filter.
 */
static inline oskit_bool_t
ipnettomedia_match(
	oskit_u32_t				ip_net_to_media_if_index,
	oskit_mib_string_t			ip_net_to_media_phys_address,
	oskit_mib_in_addr_t			ip_net_to_media_net_address,
	oskit_u32_t				ip_net_to_media_type,
	oskit_mib_slotset_ipnettomedia_t *	filter)
{
	if (!filter)
		return 1;
	
#define MATCH_SLOT(slot_type, slot_name)				\
	if (oskit_mib_##slot_type##_test(&(filter->slot_name))		\
	    && (slot_name						\
		!= oskit_mib_##slot_type##_get(&(filter->slot_name))))	\
		return 0
	
	MATCH_SLOT(u32,		ip_net_to_media_if_index);
	
	/* XXX --- This should be in an OSKit MIB library function. */
	if (oskit_mib_mib_string_test(&(filter->
					ip_net_to_media_phys_address))) {
		oskit_mib_string_t fstr =
			oskit_mib_mib_string_get(
				&(filter->ip_net_to_media_phys_address));
		
		if (fstr.len != ip_net_to_media_phys_address.len)
			return 0;
		if (memcmp(fstr.str,
			   ip_net_to_media_phys_address.str,
			   fstr.len))
			return 0;
	}
	
	MATCH_SLOT(mib_in_addr,	ip_net_to_media_net_address);
	MATCH_SLOT(u32,		ip_net_to_media_type);
#undef MATCH_SLOT
	
	return 1;
}

/*
 * Copy the string data from `src' into `dst', and mark `dst' as valid.
 *
 * XXX --- This should be some sort of OSKit MIB library function.
 */
static void
copy_out_str(
	oskit_mib_mib_string_t *	dst,
	oskit_mib_string_t *		src)
{
	size_t memcpy_len;
	
	dst->value.len = src->len;
	
	memcpy_len = src->len;
	if (memcpy_len > dst->value.max)
		memcpy_len = dst->value.max;
	memcpy(dst->value.str, src->str, memcpy_len);
	
	dst->valid = 1;
}

/*
 * This is the ``callback function'' that is invoked by `sysctl_rtsock' in
 * order to copy data out.  See the comments before `get_one_ipaddr' for
 * general information about the data we receive on each callback.
 */
static int
get_one_ipnettomedia(
	struct sysctl_req *	req,
	const void *		buf,
	size_t			buf_size)
{
	get_ipnettomedia_env_t *	env = (get_ipnettomedia_env_t *) req;
	struct ifa_msghdr *		ifam = (struct ifa_msghdr *) buf;
	
	struct rt_addrinfo	rtinfo;
	struct sockaddr_in *	sin;
	
	oskit_u32_t		if_index;
	/* oskit_mib_string_t	phys_address; --- not needed. */
	oskit_mib_in_addr_t	net_address;
	oskit_u32_t		type;
	
	/*********************************************************************/
	
	/*
	 * We get physical addresses from `RTM_IFINFO' messages, and we get IP
	 * addresses from subsequent `RTM_NEWADDR' messages.  Note that there
	 * may be more than one IP address per interface.
	 *
	 */
	if (ifam->ifam_type == RTM_IFINFO) {
		struct if_msghdr *	ifm = (struct if_msghdr *) buf;
		struct sockaddr_dl *	sdl = (struct sockaddr_dl *) (ifm+1);
		
		extract_phys_address(sdl, env);
		
		DEBUG_SET_STATE((env->state = SEEN_IFINFO));
		goto done;
	}
	
	if (ifam->ifam_type != RTM_NEWADDR) {
		assert(!"Received an unknown type of message.");
		goto done;
	}
	
	DEBUG_CHECK_STATE((env->state == SEEN_IFINFO));
	
	/*********************************************************************/
	
	/* If we don't have a valid physical address, bail early. */
	if (!env->phys_address.str)
		goto done;
	
	/*
	 * Figure out what the socket addresses mean; i.e., assign them to the
	 * appropriate elements of the `rtinfo.rti_info' array.
	 */
	rtinfo.rti_addrs = ifam->ifam_addrs;
	rt_xaddrs(((char *) (ifam + 1)),
		  ((char *) ifam) + ifam->ifam_msglen,
		  &rtinfo);
	
	/*
	 * From `env->phys_address' and `rtinfo', decode the fields of our
	 * table row.
	 */
	if_index = ifam->ifam_index;
	
	/* phys_address = env->phys_address; --- why bother? */
	
	sin = (struct sockaddr_in *) rtinfo.rti_info[RTAX_IFA];
	if (!sin)
		/* No interface address?! */
		goto done;
	net_address = sin->sin_addr.s_addr;
	
	/*
	 * XXX --- Perhaps this isn't right, but I'm not sure what is.  The
	 * UCD-SNMP code returns `static' for permanent ARP cache entries and
	 * `dynamic' for others.  But that doesn't seem right: what exactly is
	 * dynamic about a host's own IP-->Ethernet mappings?
	 */
	type = OSKIT_MIB_IP_NET_TO_MEDIA_TYPE_STATIC;
	
	/*********************************************************************/
	
	/*
	 * Process the entry.
	 */
	if (ipnettomedia_match(if_index, env->phys_address, net_address, type,
			       env->args.matching)) {
		if (env->args.start_row > 0) {
			/* Skip this row. */
			env->args.start_row--;
		} else if (env->args.want_rows > 0) {
			/* Accumulate this row. */
#define COPY_OUT(slot_type, slot_name)					\
	oskit_mib_##slot_type##_set(					\
		&(env->args.table->ip_net_to_media_##slot_name),	\
		slot_name)
			COPY_OUT(u32, if_index);
			copy_out_str(&(env->args.table->
				       ip_net_to_media_phys_address),
				     &(env->phys_address));
			COPY_OUT(mib_in_addr, net_address);
			COPY_OUT(u32, type);
#undef COPY_OUT
			(*env->args.out_rows)++;
			env->args.table++;
			env->args.want_rows--;
			
		} else {
			/* Indicate that the table filled up. */
			*env->args.more_rows = 1;
		}
	}
	
 done:
	/* Indicate to the `sysctl_rtsock' function that no error occurred. */
	return 0;
}

/*
 * The implementation of the `get_ipnettomedia_table' operation.
 */
OSKIT_COMDECL
bsdnet_mib_ip_get_ipnettomedia_table(
	oskit_mib_ip_t *				intf,
	oskit_u32_t					start_row,
	oskit_u32_t					want_rows,
	oskit_mib_slotset_ipnettomedia_t *		matching,
	/* OUT */ oskit_mib_slotset_ipnettomedia_t *	table,
	/* OUT */ oskit_u32_t *				out_rows,
	/* OUT */ oskit_bool_t *			more_rows)
{
	int			rt_iflist[] = { 0, AF_INET, NET_RT_IFLIST, 0 };
	get_ipnettomedia_env_t	env;
	int			rc;
	
	/*
	 * Set up the environment for the callback.  See the comments in
	 * `bsdnet_mib_ip_get_ipaddr_table' for general comments about how this
	 * function works.
	 */
	env.req.p		= 0;
	env.req.lock		= 0;
	env.req.oldptr		= &env;		/* Must be non-null. */
	env.req.oldlen		= sizeof(env);
	env.req.oldidx		= 0;
	env.req.oldfunc		= get_one_ipnettomedia;
	env.req.newptr		= 0;		/* Must be null. */
	env.req.newlen		= 0;
	env.req.newidx		= 0;
	env.req.newfunc		= 0;
	env.args.start_row	= start_row;
	env.args.want_rows	= want_rows;
	env.args.matching	= matching;
	env.args.table		= table;
	env.args.out_rows	= out_rows;
	*(env.args.out_rows)	= 0;
	env.args.more_rows	= more_rows;
	*(env.args.more_rows)	= 0;
	env.phys_address.str	= 0;
	env.phys_address.len	= 0;
	env.phys_address.max	= 0;
	DEBUG_SET_STATE((env.state = INIT));
	
	/*
	 * The first arg to `sysctl_rtsock' is unused.  The second names the
	 * portion of the FreeBSD MIB that we want, and the third is the number
	 * of elements in the second arg.
	 */
	rc = sysctl_rtsock(0,
			   rt_iflist,
			   (sizeof(rt_iflist) / sizeof(int)),
			   &(env.req));
	
	/*
	 * Nothing to say here about the debug state.  If there were no
	 * interfaces, we could still be in the INIT state.
	 */
	
	return (rc ? oskit_freebsd_xlate_errno(rc) : OSKIT_S_OK);
}


/*****************************************************************************/

/*
 * This is the type of the environment that is passed to the
 * `get_one_ipforward' callback function.
 */
typedef struct get_ipforward_env get_ipforward_env_t;
struct get_ipforward_env {
	/*
	 * The `sysctl' request data.  This must be the first struct member.
	 * The callback function receives a `struct sysctl_req *', and then
	 * casts it to be a pointer to this environment structure.
	 */
	struct sysctl_req	req;
	
	/*
	 * The arguments to `bsdnet_mib_udp_get_ipforward_table', which sets up
	 * the environment for the callback.
	 */
	struct {
		oskit_u32_t					start_row;
		oskit_u32_t					want_rows;
		oskit_mib_slotset_ipforward_t *			matching;
		/* OUT */ oskit_mib_slotset_ipforward_t *	table;
		/* OUT */ oskit_u32_t *				out_rows;
		/* OUT */ oskit_bool_t *			more_rows;
	} args;
};

/*
 * Determine if the information about the given IP forwarding record
 * (`ip_forward_*') matches the given filter.
 */
static inline oskit_bool_t
ipforward_match(
	oskit_mib_in_addr_t			ip_forward_dest,
	oskit_mib_in_addr_t			ip_forward_mask,
	/* oskit_s32_t				ip_forward_policy, */
	oskit_mib_in_addr_t			ip_forward_next_hop,
	oskit_s32_t				ip_forward_if_index,
	oskit_u32_t				ip_forward_type,
	oskit_u32_t				ip_forward_proto,
	/* oskit_s32_t				ip_forward_age, */
	oskit_s32_t				ip_forward_next_hop_as,
	oskit_s32_t				ip_forward_metric1,
	oskit_s32_t				ip_forward_metric2,
	oskit_s32_t				ip_forward_metric3,
	oskit_s32_t				ip_forward_metric4,
	oskit_s32_t				ip_forward_metric5,
	oskit_mib_slotset_ipforward_t *		filter)
{
	if (!filter)
		return 1;
	
#define MATCH_SLOT(slot_type, slot_name)				\
	if (oskit_mib_##slot_type##_test(&(filter->slot_name))		\
	    && (slot_name						\
		!= oskit_mib_##slot_type##_get(&(filter->slot_name))))	\
		return 0
	
	MATCH_SLOT(mib_in_addr,	ip_forward_dest);
	MATCH_SLOT(mib_in_addr,	ip_forward_mask);
	/* MATCH_SLOT(s32,		ip_forward_policy); */
	MATCH_SLOT(mib_in_addr,	ip_forward_next_hop);
	MATCH_SLOT(s32,		ip_forward_if_index);
	MATCH_SLOT(u32,		ip_forward_type);
	MATCH_SLOT(u32,		ip_forward_proto);
	/* MATCH_SLOT(s32,	ip_forward_age); */
	MATCH_SLOT(s32,		ip_forward_next_hop_as);
	MATCH_SLOT(s32,		ip_forward_metric1);
	MATCH_SLOT(s32,		ip_forward_metric2);
	MATCH_SLOT(s32,		ip_forward_metric3);
	MATCH_SLOT(s32,		ip_forward_metric4);
	MATCH_SLOT(s32,		ip_forward_metric5);
#undef MATCH_SLOT
	
	return 1;
}

/*
 * This is the ``callback function'' that is invoked by `sysctl_rtsock' in
 * order to copy data out.  On each invocation, we get a buffer containing:
 *
 *   + A `struct rt_msghdr'.  The `rtm_type' member of this structure should
 *     always be `RTM_GET'.
 *   + A set of zero or more `struct sockaddr's describing the addresses
 *     associated with the route.
 *
 * See the function `sysctl_rtsock' for further details.  Some of the code
 * below is modeled after code in the FreeBSD `route' utility; in FreeBSD, see
 * the file `/usr/src/sbin/route/route.c'.
 */
static int
get_one_ipforward(
	struct sysctl_req *	req,
	const void *		buf,
	size_t			buf_size)
{
	get_ipforward_env_t *	env = (get_ipforward_env_t *) req;
	struct rt_msghdr *	rtm = (struct rt_msghdr *) buf;
	
	struct rt_addrinfo	rtinfo;
	struct sockaddr_in *	sin;
	
	oskit_mib_in_addr_t	dest;
	oskit_mib_in_addr_t	mask;
	/* oskit_s32_t		policy; */
	oskit_mib_in_addr_t	next_hop;
	oskit_s32_t		if_index;
	oskit_u32_t		type;
	oskit_u32_t		proto;
	/* oskit_s32_t		age; */
	oskit_s32_t		next_hop_as;
	oskit_s32_t		metric1;
	oskit_s32_t		metric2;
	oskit_s32_t		metric3;
	oskit_s32_t		metric4;
	oskit_s32_t		metric5;
	
	/*********************************************************************/
	
	if (rtm->rtm_version != RTM_VERSION) {
		/* A message we don't understand?  This should never happen. */
		assert(!"Received an unknown type of message.");
		goto done;
	}
	if (rtm->rtm_type != RTM_GET) {
		/* A message we don't understand?  This should never happen. */
		assert(!"Received an unknown type of message.");
		goto done;
	}
	
	/*
	 * Figure out what the socket addresses mean; i.e., assign them to the
	 * appropriate elements of the `rtinfo.rti_info' array.
	 */
	rtinfo.rti_addrs = rtm->rtm_addrs;
	rt_xaddrs(((char *) (rtm + 1)),
		  ((char *) rtm) + rtm->rtm_msglen,
		  &rtinfo);
	
	/* From `rtinfo', decode the fields of our table row. */
	sin = (struct sockaddr_in *) rtinfo.rti_info[RTAX_DST];
	if (!sin)
		/* No destination address?! */
		goto done;
	dest = sin->sin_addr.s_addr;
	
	sin = (struct sockaddr_in *) rtinfo.rti_info[RTAX_NETMASK];
	if (!sin)
		/* No network mask?! */
		goto done;
	mask = sin->sin_addr.s_addr;
	
	/*
	 * XXX --- RFC 1354 says that this should be an encoding of the IP TOS.
	 * I don't understand how I'm supposed to determine that for a route.
	 * The TOS is associated with a particular socket, not a route, right?
	 */
	/* policy = ...; */
	
	if (rtm->rtm_flags & RTF_LOCAL) {
		next_hop = INADDR_ANY; /* See RFC 1354. */
	} else {
		sin = (struct sockaddr_in *) rtinfo.rti_info[RTAX_GATEWAY];
		if (!sin)
			/* No next hop address?! */
			goto done;
		
		switch (sin->sin_family) {
		case AF_INET:
			next_hop = sin->sin_addr.s_addr;
			break;
		case AF_LINK:
			/*
			 * The gateway address is a link-layer address.  This
			 * means that we're talking about one of our own
			 * interfaces.
			 *
			 * RFC 1213 says (for `ipRouteNextHop'): ``In the case
			 * of a route bound to an interface which is realized
			 * via a broadcast media, the value of this field is
			 * the agent's IP address on that interface.''
			 *
			 * RFC 1354 says (for `ipForwardNextHop'): ``On remote
			 * routes, the address of the next system en route;
			 * Otherwise, 0.0.0.0.''
			 *
			 * We follow RFC 1354 because we *are* implementing the
			 * `ipForwardTable' after all.  Plus, we don't have our
			 * IP address on the interface handy at this point.
			 * (`dest' isn't it.  `dest' has had the subnet mask
			 * applied to it, so it's missing parts of our addr.)
			 */
#if 1
			next_hop = INADDR_ANY;
#else
			sdl = (struct sockaddr_dl *) sin;
			switch (sdl->sdl_type) {
			case IFT_ETHER:
			/* List other broadcast media here. */
				next_hop = /* Our IP address.  Not `dest'. */;
				break;
			default:
				next_hop = INADDR_ANY;
				break;
			}
#endif
			break;
		default:
			assert(!"Received an unrecognized kind of next-hop");
			next_hop = INADDR_ANY;
			break;
		}
	}
	
	if_index = rtm->rtm_index;
	type = ((rtm->rtm_flags & RTF_GATEWAY) ?
		OSKIT_MIB_IP_FORWARD_TYPE_REMOTE :
		OSKIT_MIB_IP_FORWARD_TYPE_LOCAL);
	
	proto = ((rtm->rtm_flags & RTF_LOCAL) ?
		 OSKIT_MIB_IP_FORWARD_PROTO_LOCAL :
		 (rtm->rtm_flags & RTF_DYNAMIC) ?
		 OSKIT_MIB_IP_FORWARD_PROTO_ICMP :
		 (rtm->rtm_flags & RTF_STATIC) ?
		 OSKIT_MIB_IP_FORWARD_PROTO_NETMGMT :
		 OSKIT_MIB_IP_FORWARD_PROTO_OTHER
		 );
	
	/* XXX --- I don't believe that route age is stored anywhere. */
	/* age = ...; */
	
	next_hop_as = 0; /* See IETF RFC 1354. */
	
	/*
	 * XXX --- I don't really understand the semantics of these values;
	 * e.g., are they supposed to have well-defined meanings?  I've just
	 * picked the most interesting set of metrics from our route info.
	 */
	metric1 = (rtm->rtm_flags & RTF_UP);	/* Is this route usable? */
	metric2 = rtm->rtm_rmx.rmx_hopcount;	/* Max hops expected. */
	metric3 = rtm->rtm_rmx.rmx_sendpipe;	/* Out delay-b'width product */
	metric4 = rtm->rtm_rmx.rmx_rtt;		/* Estimated round trip time */
	metric5 = rtm->rtm_rmx.rmx_rttvar;	/* Estimated RTT variance. */
	
	/*********************************************************************/
	
	/*
	 * Process the entry.
	 */
	if (ipforward_match(dest, mask, /* policy, */ next_hop,
			    if_index, type, proto, /* age, */ next_hop_as,
			    metric1, metric2, metric3, metric4, metric5,
			    env->args.matching)) {
		if (env->args.start_row > 0) {
			/* Skip this row. */
			env->args.start_row--;
		} else if (env->args.want_rows > 0) {
			/* Accumulate this row. */
#define COPY_OUT(slot_type, slot_name)				\
	oskit_mib_##slot_type##_set(				\
		&(env->args.table->ip_forward_##slot_name),	\
		slot_name)
#define CLEAR_OUT(slot_type, slot_name)				\
	oskit_mib_##slot_type##_clear(				\
		&(env->args.table->ip_forward_##slot_name))
			COPY_OUT(mib_in_addr,	dest);
			COPY_OUT(mib_in_addr,	mask);
			CLEAR_OUT(s32,		policy);	/* XXX */
			COPY_OUT(mib_in_addr,	next_hop);
			COPY_OUT(s32,		if_index);
			COPY_OUT(u32,		type);
			COPY_OUT(u32,		proto);
			CLEAR_OUT(s32,		age);		/* XXX */
			COPY_OUT(s32,		next_hop_as);
			COPY_OUT(s32,		metric1);
			COPY_OUT(s32,		metric2);
			COPY_OUT(s32,		metric3);
			COPY_OUT(s32,		metric4);
			COPY_OUT(s32,		metric5);
#undef COPY_OUT
			(*env->args.out_rows)++;
			env->args.table++;
			env->args.want_rows--;
			
		} else {
			/* Indicate that the table filled up. */
			*env->args.more_rows = 1;
		}
	}
	
 done:
	/* Indicate to the `sysctl_rtsock' function that no error occurred. */
	return 0;
}

/*
 * The implementation of the `get_ipforward_table' operation.
 */
OSKIT_COMDECL
bsdnet_mib_ip_get_ipforward_table(
	oskit_mib_ip_t *				intf,
	oskit_u32_t					start_row,
	oskit_u32_t					want_rows,
	oskit_mib_slotset_ipforward_t *			matching,
	/* OUT */ oskit_mib_slotset_ipforward_t *	table,
	/* OUT */ oskit_u32_t *				out_rows,
	/* OUT */ oskit_bool_t *			more_rows)
{
	int			rt_iflist[] = { 0, AF_INET, NET_RT_DUMP, 0 };
	get_ipforward_env_t	env;
	int			rc;
	
	/*
	 * Set up the environment for the callback.  See the comments in
	 * `bsdnet_mib_ip_get_ipaddr_table' for general comments about how this
	 * function works.
	 */
	env.req.p		= 0;
	env.req.lock		= 0;
	env.req.oldptr		= &env;		/* Must be non-null. */
	env.req.oldlen		= sizeof(env);
	env.req.oldidx		= 0;
	env.req.oldfunc		= get_one_ipforward;
	env.req.newptr		= 0;		/* Must be null. */
	env.req.newlen		= 0;
	env.req.newidx		= 0;
	env.req.newfunc		= 0;
	env.args.start_row	= start_row;
	env.args.want_rows	= want_rows;
	env.args.matching	= matching;
	env.args.table		= table;
	env.args.out_rows	= out_rows;
	*(env.args.out_rows)	= 0;
	env.args.more_rows	= more_rows;
	*(env.args.more_rows)	= 0;
	
	/*
	 * The first arg to `sysctl_rtsock' is unused.  The second names the
	 * portion of the FreeBSD MIB that we want, and the third is the number
	 * of elements in the second arg.
	 */
	rc = sysctl_rtsock(0,
			   rt_iflist,
			   (sizeof(rt_iflist) / sizeof(int)),
			   &(env.req));
	
	/*
	 * Nothing to say here about the debug state.  If there were no
	 * interfaces, we could still be in the INIT state.
	 */
	
	return (rc ? oskit_freebsd_xlate_errno(rc) : OSKIT_S_OK);
}

OSKIT_COMDECL
bsdnet_mib_ip_set_ipforward_table(
	oskit_mib_ip_t *				intf,
	oskit_mib_slotset_ipforward_t *			matching,
	oskit_bool_t					all_matches,
	oskit_mib_slotset_ipforward_t *			ss)
{
	/* XXX */
	return OSKIT_E_NOTIMPL;
}


/*****************************************************************************/

/* End of file. */

