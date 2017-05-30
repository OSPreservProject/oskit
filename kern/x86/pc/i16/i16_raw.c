/*
 * Copyright (c) 1994-1998 University of Utah and the Flux Group.
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
	This file rovides a default implementation
	of real/pmode switching code.
	Assumes that, as far as it's concerned,
	low linear address always map to physical addresses.
	(The low linear mappings can be changed,
	but must be changed back before switching back to real mode.)

	Provides:
		i16_raw_switch_to_pmode()
		i16_raw_switch_to_real_mode()

		i16_raw_start()
			Called in real mode.
			Initializes the pmode switching system,
			switches to pmode for the first time,
			and calls the 32-bit function raw_start().

	Depends on:

		paging.h:
			raw_paging_enable()
			raw_paging_disable()
			raw_paging_init()

		a20.h:
			i16_enable_a20()
			i16_disable_a20()

		real.h:
			real_cs
*/

#include <stdio.h>
#include <stdlib.h>
#include <oskit/debug.h>
#include <oskit/x86/types.h>
#include <oskit/x86/i16.h>
#include <oskit/x86/proc_reg.h>
#include <oskit/x86/pio.h>
#include <oskit/x86/seg.h>
#include <oskit/x86/eflags.h>
#include <oskit/x86/pmode.h>
#include <oskit/x86/base_vm.h>
#include <oskit/x86/base_gdt.h>
#include <oskit/x86/base_cpu.h>
#include <oskit/x86/pc/pic.h>
#include <oskit/x86/pc/base_irq.h>
#include <oskit/x86/pc/base_i16.h>
#include <oskit/x86/pc/base_console.h>

/* Set to true when everything is initialized properly.  */
static oskit_bool_t inited;

/* Saved value of eflags register for real mode.  */
static unsigned real_eflags;



#ifdef KNIT

/*
 * This 32-bit function is the last thing called when the system
 * shuts down.  It switches back to 16-bit mode and calls i16_exit().
 */
void exit32(int rc)
{
	do_16bit(KERNEL_CS, KERNEL_16_CS,
		i16_raw_switch_to_real_mode();
		i16_exit(rc);
	);
}

/* hack, hack, hack - can't use ifdef inside a macro call so ... */
#define atexit(x) /* do nothing */

#else /* !KNIT */

/*
 * This 32-bit atexit handler is registered on startup;
 * it switches back to 16-bit mode and calls i16_exit().
 * Unfortunately we can't pass the true return code
 * because C's standard atexit mechanism doesn't provide it.
 */
static void raw_atexit(void)
{
	do_16bit(KERNEL_CS, KERNEL_16_CS,
		i16_raw_switch_to_real_mode();
		i16_exit(0);
	);
}
#endif /* !KNIT */

CODE16

void i16_raw_start(void (*start32)(void) OSKIT_NORETURN)
{
	/* Make sure we're not already in protected mode.  */
	if (i16_get_msw() & CR0_PE)
		i16_panic("The processor is in an unknown "
			  "protected mode environment.");

	do_debug(i16_puts("Real mode detected"));

	/* Minimally initialize the GDT.  */
	i16_base_gdt_init();

	/* Switch to protected mode for the first time.
	   This won't load all the processor tables and everything yet,
	   since they're not fully initialized.  */
	i16_raw_switch_to_pmode();

	/* We can now hop in and out of 32-bit mode at will.  */
	i16_do_32bit(KERNEL_CS, KERNEL_16_CS,

		/* Make sure we exit properly when we do */
		atexit(raw_atexit);

		/* Now that we can access all physical memory,
		   collect the memory regions we discovered while in 16-bit mode
		   and add them to our free memory list.
		   We can't do this before now because the free list nodes
		   are stored in the free memory itself,
		   which is probably out of reach of our 16-bit segments.  */
		/*phys_mem_collect();*/

		/* Initialize paging if necessary.
		   Do it before initializing the other processor tables
		   because they might have to be located
		   somewhere in high linear memory.  */
		/*RAW_PAGING_INIT();*/

		/* Initialize the oskit's base processor tables.  */
		base_cpu_init();

		inited = 1;

		/* Switch to real mode and back again once more,
		   to make sure everything's loaded properly.  */
		do_16bit(KERNEL_CS, KERNEL_16_CS,
			i16_raw_switch_to_real_mode();
i16_puts("switched back!");
			i16_raw_switch_to_pmode();
		);

		/* Run the 32-bit application */
		start32();
	);
}

void i16_raw_switch_to_pmode()
{
	struct pseudo_descriptor pdesc;

	/* No interrupts from now on please.  */
	i16_cli();

	/* Save the eflags register for switching back later.  */
	real_eflags = get_eflags();

	/* Enable the A20 address line.  */
	base_i16_enable_a20();

	/*
	 * Load our protected-mode GDT.
	 * Note that we have to do this each time we enter protected mode,
	 * not just the first,
	 * because other real-mode programs may have switched to pmode
	 * and back again in the meantime, trashing the GDT pointer.
	 */
	pdesc.limit = sizeof(base_gdt)-1;
	pdesc.linear_base = kvtolin(&base_gdt);
	i16_set_gdt(&pdesc);

	/* Switch into protected mode.  */
	i16_enter_pmode(KERNEL_16_CS);

	/* Reload all the segment registers from the new GDT.  */
	i16_set_ds(KERNEL_DS);
	i16_set_es(KERNEL_DS);
	i16_set_fs(0);
	i16_set_gs(0);
	i16_set_ss(KERNEL_DS);

	i16_do_32bit(KERNEL_CS, KERNEL_16_CS,

		if (inited)
		{
			/* Turn paging on if necessary.  */
			/*RAW_PAGING_ENABLE();*/

			/* Load the oskit's base CPU environment.  */
			base_cpu_load();

			/* Program the PIC so the interrupt vectors won't
			   conflict with the processor exception vectors.  */
			pic_init(BASE_IRQ_MASTER_BASE, BASE_IRQ_SLAVE_BASE);
		}

		/* Make sure our flags register is appropriate.  */
		set_eflags((get_eflags()
			   & ~(EFL_IF | EFL_DF | EFL_NT))
			   | EFL_IOPL_USER);
	);
}

void i16_raw_switch_to_real_mode()
{
	/* Make sure interrupts are disabled.  */
	cli();

	/* Avoid sending DOS bogus coprocessor exceptions.
	   XXX should we save/restore all of CR0?  */
	i16_clts();

	i16_do_32bit(KERNEL_CS, KERNEL_16_CS,
		/* Turn paging off if necessary.  */
		/*RAW_PAGING_DISABLE();*/

		/* Reprogram the PIC back to the settings DOS expects.  */
		pic_init(0x08, 0x70);
	);

	/* Make sure all the segment registers are 16-bit.
	   The code segment definitely is already,
	   because we're running 16-bit code.  */
	i16_set_ds(KERNEL_16_DS);
	i16_set_es(KERNEL_16_DS);
	i16_set_fs(KERNEL_16_DS);
	i16_set_gs(KERNEL_16_DS);
	i16_set_ss(KERNEL_16_DS);

	/* Switch back to real mode.  */
	i16_leave_pmode(real_cs);

	/* Load the real-mode segment registers.  */
	i16_set_ds(real_cs);
	i16_set_es(real_cs);
	i16_set_fs(real_cs);
	i16_set_gs(real_cs);
	i16_set_ss(real_cs);

	/* Load the real-mode IDT.  */
	{
		struct pseudo_descriptor pdesc;

		pdesc.limit = 0xffff;
		pdesc.linear_base = 0;
		i16_set_idt(&pdesc);
	}

	/* Disable the A20 address line.  */
	base_i16_disable_a20();

	/* Restore the eflags register to its original real-mode state.
	   Note that this will leave interrupts disabled
	   since it was saved after the cli() above.  */
	set_eflags(real_eflags);
}

void (*base_i16_switch_to_real_mode)() = i16_raw_switch_to_real_mode;
void (*base_i16_switch_to_pmode)() = i16_raw_switch_to_pmode;

