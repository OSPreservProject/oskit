/*
 * Copyright (c) 1996, 1998, 2000 University of Utah and the Flux Group.
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
 * PC-specific definitions for device interface.
 */

#ifndef _OSKIT_X86_PC_DEV_H_
#define _OSKIT_X86_PC_DEV_H_

#include <oskit/types.h>
#include <oskit/error.h>

/*
 * DRQ manipulation.
 */
oskit_error_t osenv_isadma_alloc(int channel);
void osenv_isadma_free(int channel);

/*
 * I/O space manipulation functions (modeled after the Linux interface).
 * osenv_io_avail() returns TRUE if the specified I/O space range is available.
 */
oskit_bool_t osenv_io_avail(oskit_addr_t port, oskit_size_t size);
oskit_error_t osenv_io_alloc(oskit_addr_t port, oskit_size_t size);
void osenv_io_free(oskit_addr_t port, oskit_size_t size);

/*
 * functions to read and write the realtime clock
 */
struct oskit_timespec;
oskit_error_t oskit_rtc_get(struct oskit_timespec *rtctime);
void oskit_rtc_set(struct oskit_timespec *rtctime);

#endif /* _OSKIT_X86_PC_DEV_H_ */
