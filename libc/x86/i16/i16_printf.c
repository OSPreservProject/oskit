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

/* This just generates 16-bit versions of the routines in printf.c.  */

#include <oskit/config.h>
#include <oskit/x86/i16.h>

/* If they don't have .code16 they simply won't have i16_printf */
#ifdef HAVE_CODE16

CODE16

#define printf			i16_printf
#define vprintf			i16_vprintf
#define _doprnt			i16__doprnt
#define puts			i16_puts
#define putchar			i16_putchar
#define console_putbytes	i16_console_putbytes
#define console_putchar		i16_console_putchar

#include "printf.c"

#endif /* HAVE_CODE16 */
