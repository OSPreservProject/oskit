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
 * This file generates a 16-bit versions of the fill_descriptor* and
 * fill_gate functions.
 */
#include <oskit/config.h>
#ifdef HAVE_CODE16

#define OSKIT_INLINE
#include <oskit/x86/i16.h>

CODE16

#define fill_descriptor i16_fill_descriptor
#define fill_descriptor_base i16_fill_descritor_base
#define fill_descriptor_limit i16_fill_descriptor_limit
#define fill_gate i16_fill_gate

#undef CODE16
#include <oskit/x86/seg.h>

#endif /* HAVE_CODE16 */
