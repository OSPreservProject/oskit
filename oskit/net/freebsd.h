/*
 * Copyright (c) 1997-1999, 2001 University of Utah and the Flux Group.
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
 * This file contains the function declarations needed for the 
 * FreeBSD derived networking code
 *
 * It mainly describes a few convenience functions to ifconfig an 
 * ethernet interface.
 *
 * Part of it is of temporary nature until a suitable COM interface for 
 * ethernet interfaces will have been defined. So far, the functionality
 * is quite limited - basically, only a local ethernet interface
 * is supported, and only a default route to one host on the attached
 * subnet can be set.
 */

#ifndef _OSKIT_NET_FREEBSD_H_
#define _OSKIT_NET_FREEBSD_H_

/* opaque types */
struct socket;
struct sockaddr;
struct ifnet;
struct oskit_freebsd_net_ether_if;

#include <oskit/machine/types.h>	
#include <oskit/dev/dev.h>
#include <oskit/dev/osenv.h>
#include <oskit/dev/device.h>
#include <oskit/dev/ethernet.h>

#include <oskit/net/socket.h>

#if FLASK
#include <flask/security.h>
#include <oskit/avc/avc.h>
#include <oskit/net/socket_secure.h>
#endif

/* initialize the bsd networking code */
oskit_error_t oskit_freebsd_net_init(oskit_osenv_t *osenv,
				     oskit_socket_factory_t **outfact);

#if FLASK
/* initialize the bsd networking code */
oskit_error_t oskit_freebsd_net_init_secure(oskit_osenv_t *osenv,
					    oskit_avc_t *avc,
					    oskit_socket_factory_t **outfact,
					    oskit_socket_factory_secure_t **outsfact);
#endif

/*
 * this contains some code to initialize a kind of standard setup
 * for a Ethernet-based TCP/IP stack
 *
 * oskit_freebsd_net_open_first_ether_if finds the FIRST (XXX!) ethernet card
 * and call oskit_freebsd_net_open_ether_if with it.
 *
 * oskit_freebsd_net_open_ether_if opens it and creates a 
 * oskit_freebsd_net_ether_if_t suitable to be passed to 
 * oskit_freebsd_net_ifconfig
 *
 * oskit_freebsd_net_close_ether_if closes a given interface
 */

oskit_error_t oskit_freebsd_net_open_first_ether_if(
	struct oskit_freebsd_net_ether_if **out_eif);

oskit_error_t oskit_freebsd_net_open_ether_if(
	struct oskit_etherdev *dev,
	struct oskit_freebsd_net_ether_if **out_eif);


/*
 * This structure contains all data associated with an interface
 */
typedef struct oskit_freebsd_net_ether_if {
        struct oskit_netio    *recv_nio;	/* set by net_prepare_ether_if */
        struct oskit_netio    *send_nio;			/* you set this */
        unsigned char   haddr[OSKIT_ETHERDEV_ADDR_SIZE];	/* you set this */
        struct ifnet    *ifp;		/* set by ifconfig */

        /* these two are optional */
        struct oskit_etherdev *dev;
        oskit_devinfo_t     info;
} oskit_freebsd_net_ether_if_t;

/*
 * partially prepare an net_ether_if structure, initializing recv_nio
 * after setting send_nio and haddr, this struct can be used just as if
 * had been obtained via oskit_freebsd_net_open_ether_if
 */
oskit_error_t
oskit_freebsd_net_prepare_ether_if(struct oskit_freebsd_net_ether_if **eif);

void oskit_freebsd_net_close_ether_if(struct oskit_freebsd_net_ether_if *);

/* ifconfig a standard local ethernet interface */
oskit_error_t  oskit_freebsd_net_ifconfig(struct oskit_freebsd_net_ether_if *eif, 
		char *name, char *ipaddr, char *netmask);

#if FLASK
oskit_error_t  oskit_freebsd_net_ifconfig_secure(
				 struct oskit_freebsd_net_ether_if *eif, 
				 char *name, char *ipaddr, char *netmask,
				 security_id_t if_sid);
#endif

/* mark an interface as down */
oskit_error_t  oskit_freebsd_net_ifdown(struct oskit_freebsd_net_ether_if *eif);

/* add a default router - must be on the local subnet */
oskit_error_t  oskit_freebsd_net_add_default_route(char *gateway);

/**********************************************************************/

/*
 * from here to the end of this file - DEPRECATED: these functions will 
 * eventually become obsolete - use the COM interfaces instead.
 */
int  bsdnet_accept(struct socket *so, 
	struct sockaddr *name, oskit_size_t *anamelen, struct socket **newso);
int  bsdnet_bind(struct socket *so, struct sockaddr *name, oskit_size_t namelen);
int  bsdnet_connect(struct socket *so, struct sockaddr *name, 
	oskit_size_t namelen);
#if FLASK
int bsdnet_connect_secure(struct socket *so, struct sockaddr *name, oskit_size_t namelen, security_id_t peer_sid);
#endif
int  bsdnet_write(struct socket *so, const void *buf, 
	oskit_size_t nbytes, oskit_u32_t *retval);
int  bsdnet_read(struct socket *so, void *buf, 
	oskit_size_t nbytes, oskit_u32_t *retval);
int  bsdnet_getsockname(struct socket *so, struct sockaddr *asa, 
	oskit_size_t *alen);
int  bsdnet_getpeername(struct socket *so, struct sockaddr *asa, 
	oskit_size_t *alen);
int  bsdnet_setsockopt(struct socket *so, oskit_u32_t level,
        oskit_u32_t name, const void *val, oskit_size_t valsize);
int bsdnet_sendto(struct socket *so, const void *msg, oskit_size_t len, 
	oskit_u32_t flags, const struct sockaddr *to, oskit_size_t tolen, 
	oskit_size_t *retval);
#if FLASK
int bsdnet_sendto_secure(struct socket *so, const void *msg, oskit_size_t len, 
	oskit_u32_t flags, const struct sockaddr *to, oskit_size_t tolen, 
	oskit_size_t *retval, security_id_t dso_sid, security_id_t msg_sid);
#endif
int bsdnet_recvfrom(struct socket *so, void *buf, oskit_size_t len, 
	oskit_u32_t flags, struct sockaddr *from, oskit_size_t *fromlen, 
	oskit_size_t *retval);
#if FLASK
int bsdnet_recvfrom_secure(struct socket *so, void *buf, oskit_size_t len, 
	oskit_u32_t flags, struct sockaddr *from, oskit_size_t *fromlen, 
	oskit_size_t *retval, security_id_t *out_sso_sid, security_id_t *out_msg_sid);
#endif

/* missing 
	getsockopt
	send/recv
	sendmsg/recvmsg
	...
 */

#endif /* _OSKIT_NET_FREEBSD_H_ */
