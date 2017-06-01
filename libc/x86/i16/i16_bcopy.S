/*
 * Copyright (c) 1994-1995, 1998, 1999 University of Utah and the Flux Group.
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

/* This just generates 16-bit versions of the routines in bcopy.S.  */

#include <oskit/config.h>

/* If they don't have .code16 they simply won't have i16_foo */
#ifdef HAVE_CODE16
#ifdef HAVE_CODE16GCC
	.code16gcc
#else
	.code16
#endif

#define l2rbcopy	i16_l2rbcopy
#define bcopy16		i16_bcopy16
#define bcopy		i16_bcopy
#define memcpy		i16_memcpy

#include "bcopy.S"

#endif /* HAVE_CODE16 */
