/*
 * Copyright (c) 1995-1998, 2000 University of Utah and the Flux Group.
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
#ifndef _OSKIT_C_TIME_H
#define _OSKIT_C_TIME_H

#include <oskit/compiler.h>
/* Include <oskit/c/sys/types.h> for `time_t', used in prototypes below. */
#include <oskit/c/sys/types.h>

OSKIT_BEGIN_DECLS

time_t time(time_t *t);

OSKIT_END_DECLS

#endif /* _OSKIT_C_TIME_H */
