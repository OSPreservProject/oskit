/*
 * Copyright (c) 1999 University of Utah and the Flux Group.
 * All rights reserved.
 * 
 * Contributed by the Computer Security Research division,
 * INFOSEC Research and Technology Office, NSA.
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

#ifndef _OSKIT_NET_SOCKET_SECURE_H_
#define _OSKIT_NET_SOCKET_SECURE_H_

#include <oskit/com.h>
#include <oskit/net/socket.h>
#include <oskit/flask/flask_types.h>

/*
 * Extensions to oskit_socket for Flask.
 * IID 4aa7dfe6-7c74-11cf-b500-08000953adc2
 */
struct oskit_socket_secure {
	struct oskit_socket_secure_ops *ops;
};
typedef struct oskit_socket_secure oskit_socket_secure_t;

struct oskit_socket_secure_ops {

	/*** COM-specified IUnknown interface operations ***/
	OSKIT_COMDECL_IUNKNOWN(oskit_socket_secure_t)

	/*** Operations specific to the oskit_socket_secure interface ***/

        /*
         * Same as oskit_socket_t accept, except that it also
	 * returns the SID of the peer socket.
         */ 
	OSKIT_COMDECL	(*accept)(oskit_socket_secure_t *s, 
			    struct oskit_sockaddr *name,
			    oskit_size_t *anamelen,
			    oskit_security_id_t *out_peer_sid,
			    struct oskit_socket **newopenso);

        /*
         * Same as oskit_socket_t connect, except that it permits
	 * the caller to specify the desired peer socket SID.
	 * For stream sockets, the connect will only succeed if the 
	 * socket with the specified name has the specified SID.
	 * For dgram sockets, subsequent sends after the connect will 
	 * only succeed if the destination socket has the specified SID, 
	 * and incoming datagrams will only be accepted from source 
	 * sockets with the specified SID.
         */ 
	OSKIT_COMDECL	(*connect)(oskit_socket_secure_t *s,
			    const struct oskit_sockaddr *name,
			    oskit_size_t namelen,
			    oskit_security_id_t peer_sid);

	/*
         * Same as oskit_socket_t listen, except that it permits
	 * the caller to specify the SID to use for sockets 
	 * created for incoming connections.  If 'newconn_sid' is
	 * specified, then it is used for all such sockets.
	 * If 'flags' is OSKIT_SO_SIDREFLECT, then the SID
	 * of the peer socket is used for incoming connections.
	 * If both 'newconn_sid' and OSKIT_SO_SIDREFLECT are
	 * specified, then OSKIT_E_INVALIDARG is returned.
	 */
	OSKIT_COMDECL	(*listen)(oskit_socket_secure_t *s, oskit_u32_t n,
			    oskit_security_id_t newconn_sid,
			    oskit_u32_t flags);

	/*
	 * Same as oskit_socket_t getsockname, except that it
	 * also returns the security class and the SID of the socket.
	 */
	OSKIT_COMDECL	(*getsockname)(oskit_socket_secure_t *s,
			    struct oskit_sockaddr *asa,
			    oskit_size_t *alen,
 		            oskit_security_class_t *out_sclass,
			    oskit_security_id_t *out_sid);

	/*
	 * Same as oskit_socket_t getpeername, except that it
	 * also returns the  SID of the peer socket.
	 */
        OSKIT_COMDECL    (*getpeername)(oskit_socket_secure_t *s,
                            struct oskit_sockaddr *asa,
                            oskit_size_t *alen,
			    oskit_security_id_t *out_peer_sid);

        /*
         * Same as oskit_socket_t sendto, except that it permits
	 * the caller to specify the desired destination socket SID
	 * and the SID to use for the message.  The send will only 
	 * succeed if the socket with the specified name has the specified 
	 * SID.  
         */ 
	OSKIT_COMDECL	(*sendto)(oskit_socket_secure_t *s, const void *msg,
			    oskit_size_t len, oskit_u32_t flags,
			    const struct oskit_sockaddr *to, 
			    oskit_size_t tolen,
			    oskit_size_t *retval,
                            oskit_security_id_t dso_sid,
                            oskit_security_id_t msg_sid);

	/*
	 * Same as oskit_socket_t recvfrom, except that it also
	 * returns the SID of the source socket and the SID of
	 * the message.
	 */
	OSKIT_COMDECL	(*recvfrom)(oskit_socket_secure_t *s, void *buf,
			    oskit_size_t len, oskit_u32_t flags,
			    struct oskit_sockaddr *from, oskit_size_t *fromlen,
			    oskit_size_t *retval,
			    oskit_security_id_t *out_sso_sid,
			    oskit_security_id_t *out_msg_sid);
};

#define oskit_socket_secure_query(io, iid, out_ihandle) \
	((io)->ops->query((oskit_socket_secure_t *)(io), (iid), (out_ihandle)))
#define oskit_socket_secure_addref(io) \
	((io)->ops->addref((oskit_socket_secure_t *)(io)))
#define oskit_socket_secure_release(io) \
	((io)->ops->release((oskit_socket_secure_t *)(io)))
#define oskit_socket_secure_accept(s, name, anamelen, sso, dso, newopenso) \
	((s)->ops->accept((oskit_socket_secure_t *)(s), (name), (anamelen), (sso), (dso), (newopenso)))
#define oskit_socket_secure_connect(s, name, namelen, dso) \
	((s)->ops->connect((oskit_socket_secure_t *)(s), (name), (namelen),(dso)))
#define oskit_socket_secure_listen(s, n, sid, flags) \
	((s)->ops->listen((oskit_socket_secure_t *)(s), (n), (sid), (flags)))
#define oskit_socket_secure_getsockname(s, asa, alen, sclass, sid) \
	((s)->ops->getsockname((oskit_socket_secure_t *)(s), (asa), (alen), (sclass), (sid)))
#define oskit_socket_secure_getpeername(f, asa, alen, sid) \
	((f)->ops->getpeername((oskit_socket_secure_t *)(f), (asa), (alen), (sid)))
#define oskit_socket_secure_sendto(s, msg, len, flags, to, tolen, retval, dso, msg_sid) \
	((s)->ops->sendto((oskit_socket_secure_t *)(s), (msg), (len), (flags), (to), (tolen), (retval), (dso), (msg_sid)))
#define oskit_socket_secure_recvfrom(s, buf, len, flags, from, fromlen, retval, sso, dso, msg_sid) \
	((s)->ops->recvfrom((oskit_socket_secure_t *)(s), (buf), (len), (flags), (from), (fromlen), (retval), (sso), (dso), (msg_sid)))

/* GUID for oskit_socket_secure interface */
extern const struct oskit_guid oskit_socket_secure_iid;
#define OSKIT_SOCKET_SECURE_IID OSKIT_GUID(0x4aa7dfe6, 0x7c74, 0x11cf, \
				0xb5, 0x00, 0x08, 0x00, 0x09, 0x53, 0xad, 0xc2)


/*
 * Extensions to the oskit_socket_factory interface for Flask.
 * IID 4aa7dfe7-7c74-11cf-b500-08000953adc2
 */
struct oskit_socket_factory_secure {
	struct oskit_socket_factory_secure_ops *ops;
};
typedef struct oskit_socket_factory_secure oskit_socket_factory_secure_t;

struct oskit_socket_factory_secure_ops {

	/*** COM-specified IUnknown interface operations ***/
	OSKIT_COMDECL_IUNKNOWN(oskit_socket_factory_secure_t)

	OSKIT_COMDECL	(*create)(oskit_socket_factory_secure_t *f,
		oskit_u32_t domain, oskit_u32_t type,
		oskit_u32_t protocol, oskit_security_id_t sid, 
		oskit_socket_t **out_socket);

	OSKIT_COMDECL	(*create_pair)(oskit_socket_factory_secure_t *f,
				       oskit_u32_t domain, oskit_u32_t type,
				       oskit_u32_t protocol,
				       oskit_security_id_t so1_sid,
				       oskit_security_id_t so2_sid,
				       oskit_socket_t **out_socket1,
				       oskit_socket_t **out_socket2);
};

/* GUID for oskit_socket_factory_secure interface */
extern const struct oskit_guid oskit_socket_factory_secure_iid;
#define OSKIT_SOCKET_FACTORY_SECURE_IID OSKIT_GUID(0x4aa7dfe7, 0x7c74, 0x11cf, \
				0xb5, 0x00, 0x08, 0x00, 0x09, 0x53, 0xad, 0xc2)

#define oskit_socket_factory_secure_query(io, iid, out_ihandle) \
	((io)->ops->query((oskit_socket_factory_secure_t *)(io), (iid), (out_ihandle)))
#define oskit_socket_factory_secure_addref(io) \
	((io)->ops->addref((oskit_socket_factory_secure_t *)(io)))
#define oskit_socket_factory_secure_release(io) \
	((io)->ops->release((oskit_socket_factory_secure_t *)(io)))
#define oskit_socket_factory_secure_create(f, d, t, p, s, out) \
	((f)->ops->create((oskit_socket_factory_secure_t *)(f), (d), (t), (p), (s), (out)))
#define oskit_socket_factory_secure_create_pair(f, d, t, p, s1, s2, out1, out2) \
	((f)->ops->create_pair((oskit_socket_factory_secure_t *)(f), (d), (t), (p), (s1), (s2), (out1), (out2)))

#endif /* _OSKIT_NET_SOCKET_SECURE_H_ */
