/*
 * Copyright (c) 1994-1996, 1998 University of Utah and the Flux Group.
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
 * Signal definitions for the Oskit standalone C library
 */
#ifndef _OSKIT_C_SIGNAL_H_
#define _OSKIT_C_SIGNAL_H_

/*
 * Pick up the basic stuff.
 */
#include <oskit/c/sys/signal.h>

/*
 * Pick up the machine-dependent sigcontext structure
 * representing the state saved when the oskit standalone C library
 * delivers a signal to an application signal handler.
 */
#include <oskit/machine/c/signal.h>

#endif /* _OSKIT_C_SIGNAL_H_ */
