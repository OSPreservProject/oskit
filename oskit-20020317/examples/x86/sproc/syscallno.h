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

#ifndef _SYSCALLNO_H_
#define _SYSCALLNO_H_

#define SYS_EXIT	0	/* return from user code */
#define SYS_PUTCHAR	1
#define SYS_GETCHAR	2
#define SYS_PRINTF	3
#define SYS_SLEEP	4
#define SYS_GETPID	5
#define SYS_PUTS	6
#define SYS_LOCK	7
#define SYS_UNLOCK	8
#define SYS_GETTID	9

#define NSYS	10

#endif /*_SYSCALLNO_H_*/
