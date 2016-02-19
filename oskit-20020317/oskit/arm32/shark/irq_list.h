/*
 * Copyright (c) 1995, 1998, 1999 University of Utah and the Flux Group.
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
 * This is just a handy file listing all the IRQ's on the PC,
 * whether they're on the master or slave PIC,
 * what interrupt line on that PIC they're connected to,
 * and a simple keyword/string describing their "standard" assignment, if any.
 * To use this, just define the 'irq()' macro however you want
 * and #include this file in the appropriate place.
 */

/*
 * If the irq macro has not been defined by now, then null it away since
 * its already too late.
 */
#ifndef irq
#define irq(a,b,c,d)
#endif

irq(master,0,0,timer)
irq(master,1,1,keyboard)
irq(master,2,2,slave)
irq(master,3,3,com2)
irq(master,4,4,com1)
irq(master,5,5,lpt2)
irq(master,6,6,fd)
irq(master,7,7,lpt1)

irq(slave,0,8,rtc)
irq(slave,1,9,)
irq(slave,2,10,)
irq(slave,3,11,)
irq(slave,4,12,)
irq(slave,5,13,coprocessor)
irq(slave,6,14,hd)
irq(slave,7,15,)

/*
 * Some standard defs. 
 */
#define IRQ_TIMER	0
#define IRQ_KEYBOARD	1
#define IRQ_SLAVE	2
#define IRQ_COM2	3
#define IRQ_COM1	4
#define IRQ_LPT2	5
#define IRQ_FD		6
#define IRQ_LPT1	7
#define IRQ_RTC		8
#define IRQ_COPROC	13
#define IRQ_HD		14

