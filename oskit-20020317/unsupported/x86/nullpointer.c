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
 * This code demonstrates how to use the debug registers to set up
 * a SIGSEGV-type null pointer trap
 *
 * All you do is to call "setup_null_pointer_trap(your_handler)" where
 * "your_handler" may be defined as
 *
 * #include <oskit/x86/base_trap.h>
 * int your_handler (struct trap_state *ts)
 * {
 *     ...
 * }
 *
 * XXX provide a way to uninstall the trap handler... 
 *
 * Warning: do not call any of the current oskit/osenv
 * initialization routines   with the null pointer stuff
 * enabled - it'll crash
 */

#include <oskit/config.h>

#ifdef HAVE_DEBUG_REGS


#include <oskit/x86/pc/com_cons.h>
#include <oskit/x86/pc/direct_cons.h>
#include <oskit/x86/pc/reset.h>
#include <oskit/c/stdio.h>
#include <oskit/c/stdlib.h>
#include <oskit/c/signal.h>
#include <oskit/c/termios.h>
#include <oskit/tty.h>
#include <oskit/gdb.h>
#include <oskit/gdb_serial.h>
#include <oskit/base_critical.h>

#include <oskit/x86/seg.h>
#include <oskit/x86/gdb.h>
#include <oskit/x86/debug_reg.h>
#include <oskit/x86/base_vm.h>
#include <oskit/x86/base_gdt.h>
#include <oskit/x86/trap.h>
#include <oskit/x86/eflags.h>
#include <oskit/x86/base_trap.h>
#include <oskit/x86/proc_reg.h>


#define VERBOSITY 1

/*
 * this function is called on/at exit following to Bryan's recommendation
 */
static
void cleanup()
{
	/*
	 * Before exiting, we need to restore the original segment descriptors
	 * because the PC reset code needs to access low BIOS memory.
	 */
	base_gdt_init();
        base_gdt_load();
}

/* a saved copy of the old handler to fall back on */
static int (*old_trap_handler)(struct trap_state *ts);

/* the installed handler */
static int (*null_trap_handler)(struct trap_state *ts);

/* my new handler */
static int my_trap(struct trap_state *ts);

/*
 * set up a pointer trap
 */
void 
setup_pointer_trap(unsigned addr)
{
	atexit(cleanup);

	/*
	 * A second way to catch null pointers
	 * is to use the processor's breakpoint registers.
	 * The disadvantage of this approach is that
	 * you can't use these debug registers for debugging...
	 */
	printf(__FUNCTION__ "(%x) called\n", addr);
#if 0
	set_b0(addr, DR7_LEN_1, DR7_RW_INST);
        set_b1(addr, DR7_LEN_4, DR7_RW_DATA);
#else
        set_b0(addr, DR7_LEN_4, DR7_RW_WRITE);	/* break on data writes */
#endif

        /*
         * The Intel Pentium manual recommends
         * executing an LGDT instruction
         * after modifying breakpoint registers.
         */
        base_gdt_load();
}

/*
 * set up a pointer trap, must be called after base trap handler
 * is set up
 */
void 
setup_null_pointer_trap(int (*handler)(struct trap_state *), void *addr)
{

	/* set up trap handlers */
	old_trap_handler = base_trap_handlers[T_DEBUG];
	base_trap_handlers[T_DEBUG] = my_trap;
	null_trap_handler = handler;
	atexit(cleanup);

	/*
	 * A second way to catch null pointers
	 * is to use the processor's breakpoint registers.
	 * The disadvantage of this approach is that
	 * you can't use these debug registers for debugging...
	 */
	set_b0(NULL, DR7_LEN_1, DR7_RW_INST);
        set_b1(NULL, DR7_LEN_4, DR7_RW_DATA);

        /*
         * The Intel Pentium manual recommends
         * executing an LGDT instruction
         * after modifying breakpoint registers.
         */
        base_gdt_load();
}

static int 
my_trap(struct trap_state *ts)
{
#if 0
	int signo;
	/*
	 * this code is saved here for later use...
	 */
        /* Convert the x86 trap code into a generic signal number.  */
        /* XXX some of these are probably not really right.  */
        switch (ts->trapno)
        {
                case -1 : signo = SIGINT; break; /* hardware interrupt */
                case 0 : signo = SIGFPE; break; /* divide by zero */
                case 1 : signo = SIGTRAP; break; /* debug exception */
                case 3 : signo = SIGTRAP; break; /* breakpoint */
                case 4 : signo = 16; break; /* into instruction (overflow) */
                case 5 : signo = 16; break; /* bound instruction */
                case 6 : signo = SIGILL; break; /* Invalid opcode */
                case 7 : signo = SIGFPE; break; /* coprocessor not available */
                case 8 : signo = 7; break; /* double fault */
                case 9 : signo = SIGSEGV; /* coprocessor segment overrun */
			break; 
                case 10 : signo = SIGSEGV; break; /* Invalid TSS */
                case 11 : signo = SIGSEGV; break; /* Segment not present */
                case 12 : signo = SIGSEGV; break; /* stack exception */
                case 13 : signo = SIGSEGV; break; /* general protection */
                case 14 : signo = SIGSEGV; break; /* page fault */
                case 16 : signo = 7; break; /* coprocessor error */
                default: signo = 7;         /* "software generated"*/
        }
#endif

	if (ts->trapno == T_DEBUG) {
#if VERBOSITY > 0
		printf("Received debugging trap..., invoking handler\n");
#endif
		if (null_trap_handler)
			null_trap_handler(ts);
#if VERBOSITY > 0
		printf("Can't handle trap %d - resorting to old handler!\n", 
			ts->trapno);
#endif
		/* begin temporary panic... */
		cleanup();
		pc_reset();
		/* end of temporary panic... */
	}
	return old_trap_handler(ts);
}

#endif /* HAVE_DEBUG_REGS */
