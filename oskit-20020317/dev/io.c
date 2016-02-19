/*
 * Copyright (c) 1996-1998 University of Utah and the Flux Group.
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
 * I/O port allocation/deallocation.
 * 0 bit indicates unallocated; 1 is allocated
 */

#include <oskit/machine/types.h>
#include <oskit/dev/dev.h>
#include <oskit/debug.h>
#include <oskit/dev/isa.h>

#ifndef NBPW
#define NBPW	32
#endif

/* Will need to fix for different ranges; this works on a PC */
#define NPORTS	65536

static unsigned port_bitmap[NPORTS / NBPW];

#define osenv_log(ignored...) ((void)0)	/* nix these */

oskit_bool_t
osenv_io_avail(oskit_addr_t port, oskit_size_t size)
{
	unsigned i, j, bit;

#ifdef DEBUG
	/* I'm paranoid here. oskit_addr_t & oskit_size_t are unsigned */
	if ((port >= NPORTS) || (size >= NPORTS) || (port + size >= NPORTS)) {
#else
	if (port + size >= NPORTS) {
#endif
		osenv_log(OSENV_LOG_WARNING,
			 "%s:%d: osenv_io_avail: bad port range [0x%x-0x%x]\n",
			 __FILE__, __LINE__, port, port+size);
		return 0;	/* can't be free */
	}

	i = port / NBPW;
	bit = 1 << (port % NBPW);
	for (j = 0; j < size; j++) {
		if (port_bitmap[i] & bit)
			return (0);
		bit <<= 1;
		if (bit == 0) {
			bit = 1;
			i++;
		}
	}
	return (1);	/* is free */
}

oskit_error_t
osenv_io_alloc(oskit_addr_t port, oskit_size_t size)
{
	unsigned i, j, bit;

	if (!osenv_io_avail(port, size)) {
		osenv_log(OSENV_LOG_WARNING,
			 "%s:%d: osenv_io_alloc: bad port range [0x%x-0x%x]\n",
			__FILE__, __LINE__, port, port+size);
		/* what error codes should this use? */
		return -1;
	}
	i = port / NBPW;
	bit = 1 << (port % NBPW);
	for (j = 0; j < size; j++) {
		port_bitmap[i] |= bit;
		bit <<= 1;
		if (bit == 0) {
			bit = 1;
			i++;
		}
	}
	return 0;
}

void
osenv_io_free(oskit_addr_t port, oskit_size_t size)
{
	unsigned i, j, bit;

#ifdef DEBUG
	if ((port >= NPORTS) || (size >= NPORTS) || (port + size >= NPORTS)) {
#else
	if (port + size >= NPORTS) {
#endif
		osenv_log(OSENV_LOG_WARNING,
			"%s:%d: osenv_io_free: bad port range [0x%x-0x%x]\n",
			 __FILE__, __LINE__, port, port+size);
		return;
	}
	i = port / NBPW;
	bit = 1 << (port % NBPW);
	for (j = 0; j < size; j++) {
		if ((port_bitmap[i] & bit) == 0) {
			osenv_log(OSENV_LOG_WARNING,
				"%s:%d: osenv_io_free: port 0x%x not allocated\n",
				__FILE__, __LINE__, port /* +j */);
		}
		port_bitmap[i] &= ~bit;
		bit <<= 1;
		if (bit == 0) {
			bit = 1;
			i++;
		}
	}
}
