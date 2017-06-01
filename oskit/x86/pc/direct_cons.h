/*
 * Copyright (c) 1996, 1998, 1999 University of Utah and the Flux Group.
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
 * The "direct console" is an extremely simple console "driver" for PCs,
 * which writes directly to CGA/EGA/VGA display memory to output text,
 * and reads directly from the keyboard using polling to read characters.
 * The putchar and getchar routines are completely independent;
 * either can be used without the other.
 */
#ifndef _OSKIT_X86_PC_DIRECT_CONS_H_
#define _OSKIT_X86_PC_DIRECT_CONS_H_

#include <oskit/compiler.h>

OSKIT_BEGIN_DECLS

int direct_cons_putchar(int c);
int direct_cons_getchar(void);
int direct_cons_trygetchar(void);
void direct_cons_set_flags(int flags);

void direct_cons_bell(void);

OSKIT_END_DECLS

#define DC_RAW		0x01
#define DC_NONBLOCK	0x02
#define DC_NO_ONLCR	0x04

#endif /* _OSKIT_X86_PC_DIRECT_CONS_H_ */
