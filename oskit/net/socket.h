/*
 * Copyright (c) 1997-1998 University of Utah and the Flux Group.
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
 * Definition of the COM-based socket interfaces
 */
#ifndef _OSKIT_NET_SOCKET_H_
#define _OSKIT_NET_SOCKET_H_

#include <oskit/com.h>
#include <oskit/io/posixio.h>
#include <oskit/com/stream.h>

struct oskit_stat;
#include <oskit/io/iovec.h>

/*
 * The following is practically a repeat of BSD's sys/socket.h file.
 * We need these definitions here to make the COM interfaces self-contained
 * Note that we try to avoid any weirdness here - it's only names.
 */
/*
 * Definitions related to sockets: types, address families, options.
 */

/*
 * Types
 */
#define	OSKIT_SOCK_STREAM	1	/* stream socket */
#define	OSKIT_SOCK_DGRAM		2	/* datagram socket */
#define	OSKIT_SOCK_RAW		3	/* raw-protocol interface */
#define	OSKIT_SOCK_RDM		4	/* reliably-delivered message */
#define	OSKIT_SOCK_SEQPACKET	5	/* sequenced packet stream */

/*
 * Option flags per-socket.
 */
#define	OSKIT_SO_DEBUG		0x0001	/* turn on debugging info recording */
#define	OSKIT_SO_ACCEPTCONN	0x0002	/* socket has had listen() */
#define	OSKIT_SO_REUSEADDR	0x0004	/* allow local address reuse */
#define	OSKIT_SO_KEEPALIVE	0x0008	/* keep connections alive */
#define	OSKIT_SO_DONTROUTE	0x0010	/* just use interface addresses */
#define	OSKIT_SO_BROADCAST	0x0020	/* permit sending of broadcast msgs */
#define	OSKIT_SO_USELOOPBACK	0x0040	/* bypass hardware when possible */
#define	OSKIT_SO_LINGER		0x0080	/* linger on close if data present */
#define	OSKIT_SO_OOBINLINE	0x0100	/* leave received OOB data in line */
#define	OSKIT_SO_REUSEPORT	0x0200	/* allow local address & port reuse */
#if FLASK
#define	OSKIT_SO_SIDREFLECT	0x0400	/* reflect the peer socket's SID */
#endif

/*
 * Additional options, not kept in so_options.
 */
#define OSKIT_SO_SNDBUF		0x1001	/* send buffer size */
#define OSKIT_SO_RCVBUF		0x1002	/* receive buffer size */
#define OSKIT_SO_SNDLOWAT	0x1003	/* send low-water mark */
#define OSKIT_SO_RCVLOWAT	0x1004	/* receive low-water mark */
#define OSKIT_SO_SNDTIMEO	0x1005	/* send timeout */
#define OSKIT_SO_RCVTIMEO	0x1006	/* receive timeout */
#define	OSKIT_SO_ERROR		0x1007	/* get error status and clear */
#define	OSKIT_SO_TYPE		0x1008	/* get socket type */
#define	OSKIT_SO_NETIO		0x1009	/* set socket netio */

/*
 * Structure used for manipulating linger option.
 */
struct	oskit_linger {
	int	l_onoff;		/* option on/off */
	int	l_linger;		/* linger time */
};

/*
 * Level number for (get/set)sockopt() to apply to socket itself.
 */
#define	OSKIT_SOL_SOCKET	0xffff		/* options for socket level */

/*
 * Address families.
 */
#define	OSKIT_AF_UNSPEC		0	/* unspecified */
#define	OSKIT_AF_LOCAL		1	/* local to host (pipes, portals) */
#define	OSKIT_AF_UNIX		OSKIT_AF_LOCAL/* backward compatibility */
#define	OSKIT_AF_INET		2	/* internetwork: UDP, TCP, etc. */
#define	OSKIT_AF_IMPLINK		3	/* arpanet imp addresses */
#define	OSKIT_AF_PUP		4	/* pup protocols: e.g. BSP */
#define	OSKIT_AF_CHAOS		5	/* mit CHAOS protocols */
#define	OSKIT_AF_NS		6	/* XEROX NS protocols */
#define	OSKIT_AF_ISO		7	/* ISO protocols */
#define	OSKIT_AF_OSI		OSKIT_AF_ISO
#define	OSKIT_AF_ECMA		8	/* european computer manufacturers */
#define	OSKIT_AF_DATAKIT		9	/* datakit protocols */
#define	OSKIT_AF_CCITT		10	/* CCITT protocols, X.25 etc */
#define	OSKIT_AF_SNA		11	/* IBM SNA */
#define OSKIT_AF_DECnet		12	/* DECnet */
#define OSKIT_AF_DLI		13	/* DEC Direct data link interface */
#define OSKIT_AF_LAT		14	/* LAT */
#define	OSKIT_AF_HYLINK		15	/* NSC Hyperchannel */
#define	OSKIT_AF_APPLETALK	16	/* Apple Talk */
#define	OSKIT_AF_ROUTE		17	/* Internal Routing Protocol */
#define	OSKIT_AF_LINK		18	/* Link layer interface */
#define	OSKIT_pseudo_AF_XTP	19	/* eXpress Transfer Protocol (no AF) */
#define	OSKIT_AF_COIP		20	/* connection-oriented IP, aka ST II */
#define	OSKIT_AF_CNT		21	/* Computer Network Technology */
#define OSKIT_pseudo_AF_RTIP	22	/* Help Identify RTIP packets */
#define	OSKIT_AF_IPX		23	/* Novell Internet Protocol */
#define	OSKIT_AF_SIP		24	/* Simple Internet Protocol */
#define	OSKIT_pseudo_AF_PIP	25	/* Help Identify PIP packets */
#define	OSKIT_AF_ISDN		26	/* Integrated Services Digital Network*/
#define	OSKIT_AF_E164		OSKIT_AF_ISDN /* CCITT E.164 recommendation */
#define OSKIT_pseudo_AF_KEY   	27      /* Internal key-management function */
#define OSKIT_AF_INET6        	28      /* IPv6 */

#define	OSKIT_AF_MAX		29

/*
 * Structure used by kernel to store most
 * addresses.
 */
struct oskit_sockaddr {
	oskit_u8_t	sa_len;		/* total length */
	oskit_u8_t	sa_family;	/* address family */
	char		sa_data[14];	/* actually longer; address value */
};

/*
 * Protocol families, same as address families for now.
 */
#define	OSKIT_PF_UNSPEC		OSKIT_AF_UNSPEC
#define	OSKIT_PF_LOCAL		OSKIT_AF_LOCAL
#define	OSKIT_PF_UNIX		OSKIT_PF_LOCAL	/* backward compatibility */
#define	OSKIT_PF_INET		OSKIT_AF_INET
#define	OSKIT_PF_IMPLINK		OSKIT_AF_IMPLINK
#define	OSKIT_PF_PUP		OSKIT_AF_PUP
#define	OSKIT_PF_CHAOS		OSKIT_AF_CHAOS
#define	OSKIT_PF_NS		OSKIT_AF_NS
#define	OSKIT_PF_ISO		OSKIT_AF_ISO
#define	OSKIT_PF_OSI		OSKIT_AF_ISO
#define	OSKIT_PF_ECMA		OSKIT_AF_ECMA
#define	OSKIT_PF_DATAKIT		OSKIT_AF_DATAKIT
#define	OSKIT_PF_CCITT		OSKIT_AF_CCITT
#define	OSKIT_PF_SNA		OSKIT_AF_SNA
#define OSKIT_PF_DECnet		OSKIT_AF_DECnet
#define OSKIT_PF_DLI		OSKIT_AF_DLI
#define OSKIT_PF_LAT		OSKIT_AF_LAT
#define	OSKIT_PF_HYLINK		OSKIT_AF_HYLINK
#define	OSKIT_PF_APPLETALK	OSKIT_AF_APPLETALK
#define	OSKIT_PF_ROUTE		OSKIT_AF_ROUTE
#define	OSKIT_PF_LINK		OSKIT_AF_LINK
				/* really just proto family, no AF */
#define	OSKIT_PF_XTP		OSKIT_pseudo_AF_XTP
#define	OSKIT_PF_COIP		OSKIT_AF_COIP
#define	OSKIT_PF_CNT		OSKIT_AF_CNT
#define	OSKIT_PF_SIP		OSKIT_AF_SIP
#define	OSKIT_PF_IPX		OSKIT_AF_IPX	/* same format as AF_NS */
				/* same format as AF_INET */
#define OSKIT_PF_RTIP		OSKIT_pseudo_AF_FTIP
#define OSKIT_PF_PIP		OSKIT_pseudo_AF_PIP
#define	OSKIT_PF_ISDN		OSKIT_AF_ISDN

#define	OSKIT_PF_MAX		OSKIT_AF_MAX

/*
 * Message header for recvmsg and sendmsg calls.
 * Used value-result for recvmsg, value only for sendmsg.
 */
struct oskit_msghdr {
	oskit_addr_t	msg_name;		/* optional address */
	oskit_u32_t	msg_namelen;		/* size of address */
	struct	oskit_iovec *msg_iov;		/* scatter/gather array */
	oskit_u32_t	msg_iovlen;		/* # elements in msg_iov */
	oskit_addr_t	msg_control;		/* ancillary data, see below */
	oskit_u32_t	msg_controllen;		/* ancillary data buffer len */
	oskit_u32_t	msg_flags;		/* flags on received message */
};

#define	OSKIT_MSG_OOB		0x1	/* process out-of-band data */
#define	OSKIT_MSG_PEEK		0x2	/* peek at incoming message */
#define	OSKIT_MSG_DONTROUTE	0x4	/* send without using routing tables */
#define	OSKIT_MSG_EOR		0x8	/* data completes record */
#define	OSKIT_MSG_TRUNC		0x10	/* data discarded before delivery */
#define	OSKIT_MSG_CTRUNC		0x20	/* control data lost before delivery */
#define	OSKIT_MSG_WAITALL	0x40	/* wait for full request or error */
#define	OSKIT_MSG_DONTWAIT	0x80	/* this message should be nonblocking */
#define	OSKIT_MSG_EOF		0x100	/* data completes connection */
#define OSKIT_MSG_COMPAT      	0x8000	/* used in sendit() */

/***** end of sys/socket.h quasi-replica *****/

/*
 * Basic socket object interface,
 * IID 4aa7dfad-7c74-11cf-b500-08000953adc2
 *
 * This interface corresponds directly to the set of POSIX/Unix operations
 * that can be done on objects on a socket.
 */
struct oskit_socket {
	struct oskit_socket_ops *ops;
};
typedef struct oskit_socket oskit_socket_t;

struct oskit_socket_ops {

	/*** COM-specified IUnknown interface operations ***/
	OSKIT_COMDECL_IUNKNOWN(oskit_socket_t)

	/*** Operations inherited from oskit_posixio_t ***/

	OSKIT_COMDECL	(*stat)(oskit_socket_t *f,
				struct oskit_stat *out_stats);
	OSKIT_COMDECL	(*setstat)(oskit_socket_t *f, oskit_u32_t mask,
				   const struct oskit_stat *stats);
	OSKIT_COMDECL	(*pathconf)(oskit_socket_t *f, oskit_s32_t option,
				    oskit_s32_t *out_val);

	/*** Operations specific to the oskit_socket interface ***/

	OSKIT_COMDECL	(*accept)(oskit_socket_t *s, struct oskit_sockaddr *name,
			    oskit_size_t *anamelen,
			    struct oskit_socket **newopenso);

	OSKIT_COMDECL	(*bind)(oskit_socket_t *s,
			    const struct oskit_sockaddr *name,
			    oskit_size_t namelen);

	OSKIT_COMDECL	(*connect)(oskit_socket_t *s,
			    const struct oskit_sockaddr *name,
			    oskit_size_t namelen);

        OSKIT_COMDECL    (*shutdown)(oskit_socket_t *f, oskit_u32_t how);

	OSKIT_COMDECL	(*listen)(oskit_socket_t *s, oskit_u32_t n);

	OSKIT_COMDECL	(*getsockname)(oskit_socket_t *s,
			    struct oskit_sockaddr *asa,
			    oskit_size_t *alen);

        OSKIT_COMDECL    (*getpeername)(oskit_socket_t *f,
                                    struct oskit_sockaddr *asa,
                                    oskit_size_t *alen);

	OSKIT_COMDECL	(*getsockopt)(oskit_socket_t *so, oskit_u32_t level,
			    oskit_u32_t name, void *val,
			    oskit_size_t *valsize);

	OSKIT_COMDECL	(*setsockopt)(oskit_socket_t *so, oskit_u32_t level,
			    oskit_u32_t name, const void *val,
			    oskit_size_t valsize);

	OSKIT_COMDECL	(*sendto)(oskit_socket_t *s, const void *msg,
			    oskit_size_t len, oskit_u32_t flags,
			    const struct oskit_sockaddr *to, oskit_size_t tolen,
			    oskit_size_t *retval);

	OSKIT_COMDECL	(*recvfrom)(oskit_socket_t *s, void *buf,
			    oskit_size_t len, oskit_u32_t flags,
			    struct oskit_sockaddr *from, oskit_size_t *fromlen,
			    oskit_size_t *retval);

	OSKIT_COMDECL    (*sendmsg)(oskit_socket_t *s,
			    const struct oskit_msghdr *msg, oskit_u32_t flags,
			    oskit_size_t *retval);

	OSKIT_COMDECL	(*recvmsg)(oskit_socket_t *s,
			    struct oskit_msghdr *msg, oskit_u32_t flags,
			    oskit_size_t *retval);
};

#define oskit_socket_query(io, iid, out_ihandle) \
	((io)->ops->query((oskit_socket_t *)(io), (iid), (out_ihandle)))
#define oskit_socket_addref(io) \
	((io)->ops->addref((oskit_socket_t *)(io)))
#define oskit_socket_release(io) \
	((io)->ops->release((oskit_socket_t *)(io)))
#define oskit_socket_stat(f, out_stats) \
	((f)->ops->stat((oskit_socket_t *)(f), (out_stats)))
#define oskit_socket_setstat(f, mask, stats) \
	((f)->ops->setstat((oskit_socket_t *)(f), (mask), (stats)))
#define oskit_socket_pathconf(f, option, out_val) \
	((f)->ops->pathconf((oskit_socket_t *)(f), (option), (out_val)))
#define oskit_socket_accept(s, name, anamelen, newopenso) \
	((s)->ops->accept((oskit_socket_t *)(s), (name), (anamelen), (newopenso)))
#define oskit_socket_bind(s, name, namelen) \
	((s)->ops->bind((oskit_socket_t *)(s), (name), (namelen)))
#define oskit_socket_connect(s, name, namelen) \
	((s)->ops->connect((oskit_socket_t *)(s), (name), (namelen)))
#define oskit_socket_shutdown(f, how) \
	((f)->ops->shutdown((oskit_socket_t *)(f), (how)))
#define oskit_socket_listen(s, n) \
	((s)->ops->listen((oskit_socket_t *)(s), (n)))
#define oskit_socket_getsockname(s, asa, alen) \
	((s)->ops->getsockname((oskit_socket_t *)(s), (asa), (alen)))
#define oskit_socket_getpeername(f, asa, alen) \
	((f)->ops->getpeername((oskit_socket_t *)(f), (asa), (alen)))
#define oskit_socket_getsockopt(so, level, name, val, valsize) \
	((so)->ops->getsockopt((oskit_socket_t *)(so), (level), (name), (val), (valsize)))
#define oskit_socket_setsockopt(so, level, name, val, valsize) \
	((so)->ops->setsockopt((oskit_socket_t *)(so), (level), (name), (val), (valsize)))
#define oskit_socket_sendto(s, msg, len, flags, to, tolen, retval) \
	((s)->ops->sendto((oskit_socket_t *)(s), (msg), (len), (flags), (to), (tolen), (retval)))
#define oskit_socket_recvfrom(s, buf, len, flags, from, fromlen, retval) \
	((s)->ops->recvfrom((oskit_socket_t *)(s), (buf), (len), (flags), (from), (fromlen), (retval)))
#define oskit_socket_sendmsg(s, msg, flags, retval) \
	((s)->ops->sendmsg((oskit_socket_t *)(s), (msg), (flags), (retval)))
#define oskit_socket_recvmsg(s, msg, flags, retval) \
	((s)->ops->recvmsg((oskit_socket_t *)(s), (msg), (flags), (retval)))

/* GUID for oskit_file interface */
extern const struct oskit_guid oskit_socket_iid;
#define OSKIT_SOCKET_IID OSKIT_GUID(0x4aa7dfad, 0x7c74, 0x11cf, \
				0xb5, 0x00, 0x08, 0x00, 0x09, 0x53, 0xad, 0xc2)

/*
 * Socket factory interface,
 * IID 4aa7dfb4-7c74-11cf-b500-08000953adc2
 *
 * This interface's sole purpose is to create sockets, akin to socket(2)
 * and socketpair(2).
 */
struct oskit_socket_factory {
	struct oskit_socket_factory_ops *ops;
};
typedef struct oskit_socket_factory oskit_socket_factory_t;

struct oskit_socket_factory_ops {

	/*** COM-specified IUnknown interface operations ***/
	OSKIT_COMDECL_IUNKNOWN(oskit_socket_factory_t)

	OSKIT_COMDECL	(*create)(oskit_socket_factory_t *f,
		oskit_u32_t domain, oskit_u32_t type,
		oskit_u32_t protocol, oskit_socket_t **out_socket);

	OSKIT_COMDECL	(*create_pair)(oskit_socket_factory_t *f,
				       oskit_u32_t domain, oskit_u32_t type,
				       oskit_u32_t protocol,
				       oskit_socket_t **out_socket1,
				       oskit_socket_t **out_socket2);
};

/* GUID for oskit_file interface */
extern const struct oskit_guid oskit_socket_factory_iid;
#define OSKIT_SOCKET_FACTORY_IID OSKIT_GUID(0x4aa7dfb4, 0x7c74, 0x11cf, \
				0xb5, 0x00, 0x08, 0x00, 0x09, 0x53, 0xad, 0xc2)

#define oskit_socket_factory_query(io, iid, out_ihandle) \
	((io)->ops->query((oskit_socket_factory_t *)(io), (iid), (out_ihandle)))
#define oskit_socket_factory_addref(io) \
	((io)->ops->addref((oskit_socket_factory_t *)(io)))
#define oskit_socket_factory_release(io) \
	((io)->ops->release((oskit_socket_factory_t *)(io)))
#define oskit_socket_factory_create(f, d, t, p, out) \
	((f)->ops->create((oskit_socket_factory_t *)(f), (d), (t), (p), (out)))
#define oskit_socket_factory_create_pair(f, d, t, p, out1, out2) \
	((f)->ops->create_pair((oskit_socket_factory_t *)(f), (d), (t), (p), (out1), (out2)))

#endif /* _OSKIT_NET_SOCKET_H_ */
