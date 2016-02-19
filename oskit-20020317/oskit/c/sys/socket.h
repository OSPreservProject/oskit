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
#ifndef _OSKIT_C_SYS_SOCKET_H_
#define	_OSKIT_C_SYS_SOCKET_H_

#include <oskit/types.h>
#include <oskit/compiler.h>
#include <oskit/net/socket.h>

/*
 * Definitions related to sockets: types, address families, options.
 * see oskit/net/socket.h for comments
 */

/*
 * Socket types
 */
#define	SOCK_STREAM	OSKIT_SOCK_STREAM
#define	SOCK_DGRAM	OSKIT_SOCK_DGRAM
#define	SOCK_SEQPACKET	OSKIT_SOCK_SEQPACKET

/*
 * Option flags per-socket.
 */
#define	SO_DEBUG	OSKIT_SO_DEBUG
#define	SO_ACCEPTCONN	OSKIT_SO_ACCEPTCONN
#define	SO_REUSEADDR	OSKIT_SO_REUSEADDR
#define	SO_KEEPALIVE	OSKIT_SO_KEEPALIVE
#define	SO_BROADCAST	OSKIT_SO_BROADCAST
#define	SO_LINGER	OSKIT_SO_LINGER
#define	SO_OOBINLINE	OSKIT_SO_OOBINLINE 

/*
 * Additional options, not kept in so_options.
 */
#define SO_SNDBUF	OSKIT_SO_SNDBUF
#define SO_RCVBUF	OSKIT_SO_RCVBUF
#define	SO_ERROR	OSKIT_SO_ERROR
#define	SO_TYPE		OSKIT_SO_TYPE

/*
 * Structure used for manipulating linger option.
 * Must be IDENTICAL with struct oskit_linger in net/socket.h.
 */
struct	linger {
	int	l_onoff;		/* option on/off */
	int	l_linger;		/* linger time */
};

/*
 * Level number for (get/set)sockopt() to apply to socket itself.
 */
#define	SOL_SOCKET	OSKIT_SOL_SOCKET

/*
 * Address families.
 */
typedef unsigned char sa_family_t;
#define AF_UNSPEC	OSKIT_AF_UNSPEC
#define	AF_UNIX		OSKIT_AF_UNIX
#define	AF_INET		OSKIT_AF_INET
#define	AF_MAX		OSKIT_AF_MAX
#define AF_INET6	OSKIT_AF_INET6

/*
 * Structure used by kernel to store most addresses.
 * This must be IDENTICAL with struct oskit_sockaddr in net/socket.h.
 */
struct sockaddr {
	unsigned char	sa_len;		/* total length */
	sa_family_t	sa_family;	/* address family */
	char		sa_data[14];	/* actually longer; address value */
};

/*
 * Message header for recvmsg and sendmsg calls.
 * Used value-result for recvmsg, value only for sendmsg.
 * This must be IDENTICAL with struct oskit_msghdr in net/socket.h.
 */
struct msghdr {
	void *		msg_name;		/* optional address */
	oskit_size_t	msg_namelen;		/* size of address */
	struct	iovec *	msg_iov;		/* scatter/gather array */
	int		msg_iovlen;		/* # elements in msg_iov */
	void *		msg_control;		/* ancillary data, see below */
	oskit_size_t	msg_controllen;		/* ancillary data buffer len */
	int		msg_flags;		/* flags on received message */
};

#define	MSG_OOB		OSKIT_MSG_OOB
#define	MSG_PEEK	OSKIT_MSG_PEEK
#define	MSG_EOR		OSKIT_MSG_EOR
#define	MSG_TRUNC	OSKIT_MSG_TRUNC
#define	MSG_CTRUNC	OSKIT_MSG_CTRUNC
#define	MSG_WAITALL	OSKIT_MSG_WAITALL

OSKIT_BEGIN_DECLS

int     accept (int, struct sockaddr *, int *);
int     bind (int, const struct sockaddr *, int);
int     connect (int, const struct sockaddr *, int); 
int     getpeername (int, struct sockaddr *, int *);
int     getsockname (int, struct sockaddr *, int *);
int     getsockopt (int, int, int, void *, int *);
int     listen (int, int);
oskit_ssize_t recv (int, void *, oskit_size_t, int);
oskit_ssize_t recvfrom (int, void *, oskit_size_t, int, struct sockaddr *, int *);
oskit_ssize_t recvmsg (int, struct msghdr *, int);
oskit_ssize_t send (int, const void *, oskit_size_t, int);
oskit_ssize_t sendto (int, const void *,
            oskit_size_t, int, const struct sockaddr *, int);
oskit_ssize_t sendmsg (int, const struct msghdr *, int);
int     setsockopt (int, int, int, const void *, int);
int     shutdown (int, int);
int     socket (int, int, int);
int     socketpair (int, int, int, int *);

OSKIT_END_DECLS

#endif /* !_OSKIT_C_SYS_SOCKET_H_ */
