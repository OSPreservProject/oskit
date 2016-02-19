/*
 * Copyright (c) 1994-1996 Sleepless Software
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
 * Remote serial-line source-level debugging for the Flux OS Toolkit.
 */
#ifndef _OSKIT_X86_PC_SERIAL_GDB_H_
#define _OSKIT_X86_PC_SERIAL_GDB_H_

#include <oskit/compiler.h>

extern int serial_gdb_enable;

OSKIT_BEGIN_DECLS
extern void serial_gdb_init();
extern int serial_gdb_signal(int *inout_sig, struct trap_state *ts);
extern void serial_gdb_shutdown(int exitcode);
OSKIT_END_DECLS

#endif /* _OSKIT_X86_PC_SERIAL_GDB_H_ */
