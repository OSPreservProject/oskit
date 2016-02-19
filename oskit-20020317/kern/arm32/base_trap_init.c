/*
 * Copyright (c) 1999 University of Utah and the Flux Group.
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

#include <oskit/arm32/base_vm.h>
#include <oskit/arm32/base_trap.h>
#include <oskit/arm32/trap.h>
#include <oskit/arm32/proc_reg.h>

extern void		set_stackptr(int, unsigned int *);
extern unsigned int	abort_stack[], fiq_stack[], irq_stack[], undef_stack[];
extern unsigned int     base_trap_vectors[];

void base_trap_init(void)
{
	set_stackptr(PSR_ABT32_MODE, abort_stack);
	set_stackptr(PSR_FIQ32_MODE, fiq_stack);
	set_stackptr(PSR_IRQ32_MODE, irq_stack);
	set_stackptr(PSR_UND32_MODE, undef_stack);

	disable_interrupts();

#ifdef  OSKIT_ARM32_SHARK
	/*
	 * Use the OFW trap vector in page 0. Just redefine the entrypoints
	 * to use our lowlevel trap routines instead of the OFW routines.
	 */
	{
		unsigned int *vectors = (unsigned int *)0;
		int	     i;

		/*
		 * Reset is special on the shark. For one, the reset pin
		 * is not tied to the reset vector, so no point in taking
		 * it. Also, it holds the address of the reset address in
		 * the ROM, which you call to reset the machine. 
		 */
		for (i = 1; i < 8; i++) {
			vectors[i + 16] = base_trap_vectors[i];
		}

		/*
		 * Have to deal with the caches!
		 */
		clean_cache_entry_D(&vectors[16]);
		drain_write_buffer();
		flush_cache_I();
	}
#else
#error  "Must complete this section!"
#endif 
}

