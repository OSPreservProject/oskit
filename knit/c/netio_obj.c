/*
 * Copyright (c) 2000 University of Utah and the Flux Group.
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

#include <oskit/dev/ethernet.h>
#include <oskit/dev/dev.h>
#include <oskit/io/netio.h>

extern oskit_error_t push(void *data, oskit_bufio_t *b, oskit_size_t pkt_size);
extern void* push_data;

oskit_netio_t *nio = 0;

oskit_error_t
init(void)
{
 	nio = oskit_netio_create(push, push_data);
        return (!nio);
}


oskit_error_t
fini(void)
{
        if (nio) {
                oskit_netio_release(nio);
                nio = 0;
        }
        return 0;
}
