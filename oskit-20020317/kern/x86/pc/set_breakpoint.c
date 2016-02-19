/*
 * Copyright (c) 1994-1996 Sleepless Software
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
 * Remote PC serial-line debugging for the Flux OS Toolkit
 */

#include <oskit/x86/base_gdt.h>
#include <oskit/x86/debug_reg.h>
#include <oskit/config.h>

#define NULL 0

#ifdef HAVE_DEBUG_REGS
void
gdb_pc_set_bkpt(void *bkpt)
{
	/*
	 * Set up the debug registers
	 * to catch null pointer references,
	 * and to take a breakpoint at the entrypoint to main().
	 */
	set_b0(NULL, DR7_LEN_1, DR7_RW_INST);
	set_b1(NULL, DR7_LEN_4, DR7_RW_DATA);
	set_b2((unsigned)bkpt, DR7_LEN_1, DR7_RW_INST);

	/*
	 * The Intel Pentium manual recommends
	 * executing an LGDT instruction
	 * after modifying breakpoint registers,
	 * and experience shows that this is necessary.
	 */
	base_gdt_load();
}
#endif /* HAVE_DEBUG_REGS */
