/*
 * Copyright (c) 1996-2000 The University of Utah and the Flux Group.
 * 
 * This file is part of the OSKit Linux Glue Libraries, which are free
 * software, also known as "open source;" you can redistribute them and/or
 * modify them under the terms of the GNU General Public License (GPL),
 * version 2, as published by the Free Software Foundation (FSF).
 * 
 * The OSKit is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GPL for more details.  You should have
 * received a copy of the GPL along with the OSKit; see the file COPYING.  If
 * not, write to the FSF, 59 Temple Place #330, Boston, MA 02111-1307, USA.
 */


/* defined in irq.c */               

int request_irq(unsigned int irq, 
	void (*handler)(int, void *, struct pt_regs *),
	unsigned long flags, const char *device, void *dev_id);
void free_irq(unsigned int irq, void *dev_id);
void disable_irq(unsigned int irq);
void enable_irq(unsigned int irq);
unsigned long probe_irq_on (void);
int probe_irq_off (unsigned long irqs);

/* defined in softintr.c */
extern int softintr_vector;
