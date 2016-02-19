/*
 * Copyright (c) 1996, 1998 University of Utah and the Flux Group.
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
 * This module implements a simple "global critical region"
 * used throughout the OS toolkit to ensure proper serialization
 * for various "touchy" but non-performance critical activities
 * such as panicking, rebooting, debugging, etc.
 * This critical region can safely be entered recursively;
 * the only requirement is that "enter"s match exactly with "leave"s.
 *
 * The implementation of this module is machine-dependent,
 * and generally disables interrupts and, on multiprocessors,
 * grabs a recursive spin lock.
 */
#ifndef _OSKIT_BASE_CRITICAL_H_
#define _OSKIT_BASE_CRITICAL_H_

#include <oskit/compiler.h>

OSKIT_BEGIN_DECLS
void base_critical_enter(void);
void base_critical_leave(void);
OSKIT_END_DECLS

#endif /* _OSKIT_BASE_CRITICAL_H_ */
