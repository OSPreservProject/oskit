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
 * Code to reset the Shark. 
 */

#include <oskit/arm32/base_vm.h>
#include <oskit/arm32/proc_reg.h>
#include <oskit/arm32/reset.h>

#if 1
#include <oskit/arm32/ofw/ofw.h>

void
arm32_reset()
{
	OF_exit();
}
#else
/*
 * This *should* work, but does not.
 */
/*
 * So, the reset trap vector in page 0 holds the address in ROM of the
 * reset routine. Need to transfer control there, but for some reason the
 * instruction is a "blge" instruction, so we have to munge the
 * condition codes so the branch actually happens. But, more important is
 * that the address is in ROM, and so we have to turn the MMU off.
 */
static void
really_reset()
{
	set_spsr(get_cpsr() & ~PSR_CCMASK);
	set_cpuctrl(get_cpuctrl() & ~0x180f);

	asm volatile ("movs pc, #0x8000000");

	while (1)
		;
}

void
arm32_reset()
{
	unsigned int	physaddr = kvtophys((unsigned int) really_reset);

	((void(*)(void))physaddr)();
}
#endif
