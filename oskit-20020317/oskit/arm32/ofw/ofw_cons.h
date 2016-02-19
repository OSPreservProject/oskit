/*
 * Copyright (c) 1994-1996, 1999 Sleepless Software
 * Copyright (c) 1997-1999 University of Utah and the Flux Group.
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
 * Simple polling serial console for the Flux OS toolkit
 */
#ifndef _OSKIT_ARM32_OFW_COM_CONS_H_
#define _OSKIT_ARM32_OFW_COM_CONS_H_

#include <oskit/compiler.h>

struct termios;

OSKIT_BEGIN_DECLS
/*
 * This routine must be called once to initialize the OFW console interface.
 * The supplied termios structure indicates the baud rate and other settings.
 * If com_params is NULL, a default of 9600,8,N,1 is used.
 */
void ofw_cons_init(struct termios *com_params);

/*
 * Primary character I/O routines.
 */
int ofw_cons_getchar(void);
int ofw_cons_trygetchar(void);
void ofw_cons_putchar(int ch);

OSKIT_END_DECLS

#endif /* _OSKIT_ARM32_OFW_COM_CONS_H_ */
