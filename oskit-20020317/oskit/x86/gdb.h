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
 *	Loosely based on i386-stub.c from GDB 4.14
 */
/*
 * Definitions for remote GDB debugging of x86 programs.
 */
#ifndef _OSKIT_X86_GDB_H_
#define _OSKIT_X86_GDB_H_

/* This structure represents the x86 register state frame as GDB wants it.  */
struct gdb_state
{
	unsigned	eax;
	unsigned	ecx;
	unsigned	edx;
	unsigned	ebx;
	unsigned	esp;
	unsigned	ebp;
	unsigned	esi;
	unsigned	edi;
	unsigned	eip;
	unsigned	eflags;
	unsigned	cs;
	unsigned	ss;
	unsigned	ds;
	unsigned	es;
	unsigned	fs;
	unsigned	gs;
};

struct trap_state;
struct termios;

/*
 * This source file defines a function, gdb_trap_ss(),
 * which is functionally identical to gdb_trap(), except:
 *	- It _must_ be called with the trap_frame
 *	  directly after the return address and state pointer on the stack,
 *	  rather than buried deeper down somewhere.
 *	- If the trap was from supervisor mode,
 *	  it switches and copies the trap frame to a special stack
 *	  so that GDB can safely modify the stack pointer,
 *	  e.g., when calling a function from GDB.
 */
int gdb_trap_ss(struct trap_state *inout_trap_state);

/*
 * While copying data to or from arbitrary locations,
 * the gdb_copyin() and gdb_copyout() routines set this address
 * so they can recover from any memory access traps that may occur.
 * On receiving a trap, if this address is non-null,
 * gdb_trap() simply sets the EIP to it and resumes execution,
 * at which point the gdb_copyin/gdb_copyout routines return an error.
 * If the default gdb_copyin/gdb_copyout routines are overridden,
 * the replacement routines can use this facility as well.
 */
extern unsigned gdb_trap_recover;

/*
 * Bitwise mask to disable sending signals by hardware interrupts.
 * If bit n is set, hardware interrupt n is not sent to GDB.
 */
extern unsigned gdb_trap_mask;

/*
 * Use this macro to insert a breakpoint manually anywhere in your code.
 */
#define gdb_breakpoint() asm volatile("int $3" : : : "memory");


/*
 * Set up serial-line debugging over a COM port
 */
void gdb_pc_com_init(int com_port, struct termios *com_params);

#endif /* _OSKIT_X86_GDB_H_ */
