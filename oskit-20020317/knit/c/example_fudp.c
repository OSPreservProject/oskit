/*
 * Copyright (c) 1998-2000 The University of Utah and the Flux Group.
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
 * Sends the contents of the `msg' string to `port' on `ip_addr'.
 * The other machine should be running the `fudp_recv' example to
 * receive the message.
 */

const char *msg = "The OSKit FUDP example works.";

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <oskit/net/socket.h>

#include <oskit/c/stdio.h>

extern oskit_socket_t *socket;
extern char           *ip_address;
extern in_port_t       port;

/*
 * Send one byte on socket `socket'.
 */
static void
sendone(unsigned char c, oskit_socket_t *socket)
{
	struct sockaddr_in sin;
	oskit_size_t sent;
	oskit_error_t err;

	sin.sin_len = htons(sizeof sin);
	sin.sin_family = OSKIT_AF_INET;
	sin.sin_port = htons(port);
	sin.sin_addr.s_addr = inet_addr(ip_address);

	/* Send a datagram. */
	err = oskit_socket_sendto(socket, &c, 1, 0,
				  (struct oskit_sockaddr *)&sin, sizeof sin,
				  &sent);

	printf("sendone %c\n", c);
	if (sent != 1)
		printf("sendto: only sent %d\n", sent);
	if (err)
		printf("sendto: err 0x%08x\n", err);
}

int
main(int argc, char **argv)
{
	const char *p;

	p = msg;
	while (*p)
		sendone(*p++, socket);
	sendone('\0', socket);

	return 0;
}
