/*
 * Copyright (c) 1996, 1998 University of Utah and the Flux Group.
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
 * Code to reset a PC.
 */

#include <oskit/x86/seg.h>
#include <oskit/x86/proc_reg.h>
#include <oskit/x86/base_vm.h>
#include <oskit/x86/pc/keyboard.h>
#include <oskit/x86/pc/reset.h>

static struct pseudo_descriptor null_pdesc;

void pc_reset()
{
	int i;

	/* Inform BIOS that this is a warm boot.  */
	*(unsigned short *)phystokv(0x472) = 0x1234;

	/* Try to reset using the keyboard controller.  */
	for (i = 0; i < 100; i++)
	{
		kb_command(KC_CMD_PULSE & ~KO_SYSRESET);
	}

	/* If that fails, try the ultimate kludge:
	   clearing the IDT and causing a "triple fault"
	   so that the processor is forced to reset itself.  */
	set_idt(&null_pdesc);
	asm("int $3");
}

