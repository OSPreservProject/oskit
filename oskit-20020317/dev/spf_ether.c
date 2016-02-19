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
 * Useful front-end for filtering on just the ethernet type.
 */

#include <oskit/dev/dev.h>
#include <oskit/dev/net.h>
#include <oskit/io/netio.h>
#include <oskit/c/string.h>
#include <oskit/c/stdlib.h>

#include <oskit/c/arpa/inet.h>

#include <oskit/net/ether.h>
#include <oskit/io/spf.h>

int     
oskit_netio_spf_ethertype_create(oskit_u16_t ethertype,
                oskit_netio_t *out_netio, oskit_netio_t **push_netio)
{
	oskit_u16_t 	type = htons(ethertype);

        return oskit_netio_spf_create((char *)&type, sizeof type, 
			sizeof (struct ether_header) - sizeof type /* 12 */,
			out_netio, push_netio);
}


