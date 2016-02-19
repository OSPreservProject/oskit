/*
 * Copyright (c) 1996-2000 University of Utah and the Flux Group.
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
 * Individual IRQ management.
 */

extern int fdev_irq_flags;
extern int fdev_intr_enabled;

#include <oskit/dev/dev.h>
#include <oskit/arm32/shark/pic.h>
#include <oskit/arm32/proc_reg.h>

inline void
osenv_irq_enable(int irq)
{
	pic_enable_irq(irq);
}

inline void
osenv_irq_disable(int irq)
{
	pic_disable_irq(irq);
}

inline int
osenv_irq_pending(int irq)
{
	return pic_test_irq(irq);
}

inline int
osenv_irq_enabled(int irq)
{
	return (pic_get_irqmask() & (1 << irq)) == 0;
}
