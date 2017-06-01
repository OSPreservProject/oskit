/*
 * Copyright (c) 2001 The University of Utah and the Flux Group.
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

#ifndef _OSKIT_SPROC_INTERNAL_H_
#define _OSKIT_SPROC_INTERNAL_H_

#include <oskit/c/assert.h>
#include <oskit/threads/pthread.h>
#include <oskit/sproc.h>

extern pthread_key_t oskit_sproc_thread_key;
extern void	oskit_sproc_machdep_init(void);
extern void	oskit_sproc_uvm_handler(oskit_vmspace_t vm, int signo,
					struct trap_state *frame);

extern int		oskit_sproc_debug;
#define XPRINTF(mask, args...)	do {		\
    if (mask & oskit_sproc_debug) printf(args);	\
} while (0)
#define OSKIT_DEBUG_SWITCH	0x01
#define OSKIT_DEBUG_LOADER	0x02

#endif /*_OSKIT_SPROC_INTERNAL_H_*/
