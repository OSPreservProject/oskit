/*
 * Copyright (c) 1996-1999 University of Utah and the Flux Group.
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
/*-
 * Copyright (c) 1993 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/*
 *
 * Register definitions for the Intel 8253 Programmable Interval Timer.
 *
 * This chip has three independent 16-bit down counters that can be
 * read on the fly.  There are three mode registers and three countdown
 * registers.  The countdown registers are addressed directly, via the
 * first three I/O ports.  The three mode registers are accessed via
 * the fourth I/O port, with two bits in the mode byte indicating the
 * register.  (Why are hardware interfaces always so braindead?).
 *
 * To write a value into the countdown register, the mode register
 * is first programmed with a command indicating the which byte of
 * the two byte register is to be modified.  The three possibilities
 * are load msb (TMR_MR_MSB), load lsb (TMR_MR_LSB), or load lsb then
 * msb (TMR_MR_BOTH).
 *
 * To read the current value ("on the fly") from the countdown register,
 * you write a "latch" command into the mode register, then read the stable
 * value from the corresponding I/O port.  For example, you write
 * TMR_MR_LATCH into the corresponding mode register.  Presumably,
 * after doing this, a write operation to the I/O port would result
 * in undefined behavior (but hopefully not fry the chip).
 * Reading in this manner has no side effects.
 *
 * The outputs of the three timers are connected as follows:
 *
 *	 timer 0 -> irq 0
 *	 timer 1 -> dma chan 0 (for dram refresh)
 * 	 timer 2 -> speaker (via keyboard controller)
 *
 * Timer 0 is used to call hardclock.
 * Timer 2 is used to generate console beeps.
 */
#ifndef _OSKIT_X86_PC_PIT_H_
#define _OSKIT_X86_PC_PIT_H_

/*
 * I/O port addresses of the PIT registers.
 */
#define	PIT_CNTR0	0x40	/* timer 0 counter port */
#define	PIT_CNTR1	0x41	/* timer 1 counter port */
#define	PIT_CNTR2	0x42	/* timer 2 counter port */
#define	PIT_CONTROL	0x43	/* timer control port */

/*
 * Control register bit definitions
 */
#define	PIT_SEL0	0x00	/* select counter 0 */
#define	PIT_SEL1	0x40	/* select counter 1 */
#define	PIT_SEL2	0x80	/* select counter 2 */
#define	PIT_INTTC	0x00	/* mode 0, intr on terminal cnt */
#define	PIT_ONESHOT	0x02	/* mode 1, one shot */
#define	PIT_RATEGEN	0x04	/* mode 2, rate generator */
#define	PIT_SQWAVE	0x06	/* mode 3, square wave */
#define	PIT_SWSTROBE	0x08	/* mode 4, s/w triggered strobe */
#define	PIT_HWSTROBE	0x0a	/* mode 5, h/w triggered strobe */
#define	PIT_LATCH	0x00	/* latch counter for reading */
#define	PIT_LSB		0x10	/* r/w counter LSB */
#define	PIT_MSB		0x20	/* r/w counter MSB */
#define	PIT_16BIT	0x30	/* r/w counter 16 bits, LSB first */
#define	PIT_BCD		0x01	/* count in BCD */

/*
 * Clock speed at which the standard interval timers in the PC are driven,
 * in hertz (ticks per second) and nanoseconds per tick, respectively.
 */
#define PIT_HZ		1193182
#define PIT_NS		(1000000000 / PIT_HZ)


/*
 * Macros to read and set individual PIT timers.
 */
#define pit_read(n)							\
	({	int value;						\
		outb(PIT_CONTROL, PIT_SEL##n);				\
		value = inb(PIT_CNTR##n) & 0xff;			\
		value |= (inb(PIT_CNTR##n) & 0xff) << 8;		\
		value;							\
	})
#define pit_set(n, mode, value)						\
	({	outb(PIT_CONTROL, PIT_SEL##n | PIT_16BIT | (mode));	\
		outb(PIT_CNTR##n, (value) & 0xff);			\
		outb(PIT_CNTR##n, ((value) >> 8) & 0xff);		\
	})

/*
 * Initialize timer 0 in the standard way as a rate generator
 * to act as a system clock operating at the specified frequency.
 */
#define pit_init(hz)	pit_set(0, PIT_RATEGEN, PIT_HZ / (hz))

#endif /* _OSKIT_X86_PC_PIT_H_ */
