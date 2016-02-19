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
 * Definitions relating to manipulation of the A20 address line on PCs.
 * The A20 line is generally disabled by default on system startup,
 * for compatibility with really really old 16-bit DOS software;
 * it has to be enabled before touching any memory above 1MB.
 * In general, this is only a concern for oskit programs
 * that need to start out in 16-bit real mode from the BIOS or DOS.
 */
#ifndef _OSKIT_X86_PC_A20_H_
#define _OSKIT_X86_PC_A20_H_

/*
 * Keyboard stuff for turning on the A20 address line (gak!).
 * KB_ENABLE_A20 is the magic keyboard command to enable the A20 line;
 * KB_DISABLE_A20 is the magic command to disable it.
 */
#define KB_ENABLE_A20	0xdf	/* Linux and my BIOS uses this,
				   and I trust them more than Mach 3.0,
				   but I'd like to know what the difference is
				   and if it matters.  */
			/*0x9f*/	/* enable A20,
					   enable output buffer full interrupt
					   enable data line
					   disable clock line */
#define KB_DISABLE_A20	0xdd


/*
 * Routines to test and modify the A20 line enable.
 * Currently only 16-bit versions are provided
 * because so far only 16-bit code has needed to do this.
 * (Makes some sense - who wants to run in 32-bit protected mode
 * with only the low 1MB of memory available for use?)
 */
int i16_test_a20(void);
void i16_enable_a20(void);
void i16_disable_a20(void);

#endif /* _OSKIT_X86_PC_A20_H_ */
