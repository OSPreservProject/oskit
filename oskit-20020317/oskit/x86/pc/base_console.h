/*
 * Copyright (c) 1996-1999 University of Utah and the Flux Group.
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
#ifndef _OSKIT_X86_PC_BASE_CONSOLE_H_
#define _OSKIT_X86_PC_BASE_CONSOLE_H_

#include <oskit/console.h>
#include <oskit/com/stream.h>

extern oskit_stream_t *console; /* Currently selected console */

extern int serial_console;	/* 1 if using serial console */
extern int enable_gdb;		/* 1 if using kernel GDB */

extern int cons_com_port;	/* COM port for serial console */
extern int gdb_com_port;	/* COM port for remote GDB */

/*
 * This function initializes the base console
 * using various command-line flags and environment variables.
 */
extern void base_console_init(int argc, char **argv);

#endif /* _OSKIT_X86_PC_BASE_CONSOLE_H_ */
