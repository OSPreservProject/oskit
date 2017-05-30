/*
 * Copyright (c) 1998, 1999, 2000 University of Utah and the Flux Group.
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
 * Copyright (c) 1983, 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
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

#if !defined(lint) && defined(LIBC_SCCS)
static char sccsid[] = "@(#)gmon.c	8.1 (Berkeley) 6/4/93";
#endif

#include <oskit/time.h>
#include <sys/time.h>
#include <oskit/c/sys/gmon.h>

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

#include <oskit/debug.h>
#include <oskit/dev/dev.h>
#include <oskit/x86/base_gdt.h>
#include <oskit/x86/base_idt.h>
#include <oskit/x86/base_trap.h>
#include <oskit/x86/pio.h>
#include <oskit/x86/proc_reg.h>
#include <oskit/x86/pc/base_irq.h>
#include <oskit/x86/pc/irq_list.h>

#include <oskit/x86/pc/pit.h>
#include <oskit/x86/pc/pic.h>
#include <oskit/x86/pc/rtc.h>

#ifdef GPROF

int gprof_hz;
extern struct gmonparam _gmonparam;

void
gprof_hook(void *data)
{
	struct trap_state *ts = (struct trap_state *) data;
	struct gmonparam *p;
	unsigned int i;
	unsigned int eip = ts->eip;
	int savedstate;

	p = &_gmonparam;
	savedstate = p->state;

	/* Always go busy so we don't instrument this. */
	p->state = GMON_PROF_BUSY;
	if (savedstate == GMON_PROF_ON || savedstate == GMON_PROF_BUSY) {
		i = eip - p->lowpc;
		if (i < p->textsize) {
			i /= HISTFRACTION * sizeof(*p->kcount);
			p->kcount[i]++;
		}
	}

	/* ack the RTC */
	rtcin(RTC_INTR);
	
	p->state = savedstate;
}

int
profil(char *samples, int size, int offset, int scale)
{
	static int	installed;
	char		freqcount;
	
	if (scale == 0) {
	        pic_disable_irq(IRQ_RTC);
		return 0;
	}

	cli();

	/*
	 * Use the realtime IRQ support, since that does everything
	 * we need oh so nicely. Your program must be linked with the
	 * realtime library instead of the device library.
	 */
	if (! installed) {
		oskit_error_t	err;
		
		err = osenv_irq_alloc(IRQ_RTC, gprof_hook, 0,
				      OSENV_IRQ_TRAPSTATE|OSENV_IRQ_REALTIME);
		
		if (err)
			panic("profil: Could not add realtime IRQ\n");

		installed = 1;
	}
	pic_disable_irq(IRQ_RTC);

	/*
	 * XXX RTC interaction.  Some day there may be an RTC driver
	 * with which we can interact and this can be replaced.
	 */

	/*
	 * tell the RTC clock the frequency we need
	 *
	 * 0x29 = 128Hz  0x28 = 256Hz  0x27 = 512 Hz ... 0x23 = 8192Hz
	 */
	if (gprof_hz == 0)
		gprof_hz = OSKIT_PROFHZ;

	switch(gprof_hz)
	{
	case 8192:
		freqcount = 0x23;
		break;
	case 4096:
		freqcount = 0x24;
		break;
	case 2048:
		freqcount = 0x25;
		break;
	case 1024:
		freqcount = 0x26;
		break;
	case 512:
		freqcount = 0x27;
		break;
	case 256:
		freqcount = 0x28;
		break;
	case 128:
		freqcount = 0x29;
		break;
	default:
		printf("profil:  Invalid frequency requested for OSKIT_PROFHZ.  Using 1024\n");
		freqcount = 0x26;
		break;
	}
	outb_p(0x70, 0x0a); /* select register 0x0a - the frequency counter */
	outb_p(0x71, freqcount);
	outb_p(0x70, 0x0b); /* select register 0x0b - the interrupt selector */
	outb_p(0x71, 0x42);

	/* Clear any pending interrupts in the RTC */
	pic_enable_irq(IRQ_RTC);
	outb_p(0x70, 0x0c);
	inb_p(0x71);
	pic_enable_irq(IRQ_RTC);

	sti();

	return 0; /* happy success all the time */
}

#endif /* #ifdef GPROF */

