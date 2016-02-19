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
 * This file contains definitions for the OSKIT's standard facilities
 * for setting up the base 32-bit environment in 16-bit real mode
 * and switching from real to protected mode using various techniques.
 */
#ifndef _OSKIT_X86_PC_BASE_I16_H_
#define _OSKIT_X86_PC_BASE_I16_H_

#include <oskit/compiler.h>

struct trap_state;


/*
 * This variable holds the initial real-mode code segment we had
 * when we first entered the program in 16-bit mode.
 */
extern unsigned short real_cs;

/*
 * Vectors to the functions to use to enable and disable the A20 address line.
 * By default these point to the "raw" routines in x86/pc/i16_a20.c.
 * They can be revectored to other routines,
 * e.g. to use a HIMEM driver's facilities.
 */
extern void (*base_i16_enable_a20)(void);
extern void (*base_i16_disable_a20)(void);

/*
 * Vectors to functions to switch between real and protected mode.
 * By default, these point to the raw routines in x86/pc/i16_raw.c,
 * but can be revectored to other alternative routines.
 */
extern void (*base_i16_switch_to_real_mode)();
extern void (*base_i16_switch_to_pmode)();

/*
 * Vector to the 32-bit function to use to make a BIOS or DOS call
 * in real/v86 mode through a software interrupt.
 * The trapno field in the trap_state contains the interrupt number.
 * Only the general registers, eflags, v86_ds, and v86_es registers
 * in the trap state are used and modified by this routine.
 */
extern void (*base_real_int)(struct trap_state *state);


/*
 * Raw mode switching functions.
 */
void i16_raw_switch_to_pmode(void);
void i16_raw_switch_to_real_mode(void);
void i16_raw_start(void (*start)(void) OSKIT_NORETURN);


/*
 * 16-bit function to dispatch a software interrupt from real mode,
 * using the vector number and register state in the passed trap_state.
 * Only the general registers, eflags, v86_ds, and v86_es registers
 * in the trap state are used and modified by this routine.
 */
void i16_real_int(struct trap_state *ts);


#endif /* _OSKIT_X86_PC_BASE_I16_H_ */
