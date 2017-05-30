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
#include <oskit/dev/dev.h>
#include "osenv.h"

/*
 * XXX: x86-specific.
 */
#define IF_FLAG 0x00000200

void
linux_cli()
{
	osenv_intr_disable();
}

void
linux_sti()
{
	osenv_intr_enable();
}

unsigned
linux_save_flags()
{
	unsigned rc;

	asm volatile("
		pushfl
		popl %0" : "=r" (rc));

	rc &= ~IF_FLAG;
	if (osenv_intr_enabled())
		rc |= IF_FLAG;
	return rc;
}

void
linux_restore_flags(unsigned flags)
{
	/*
	 * XXX: Linux drivers only count on the interrupt enable bit
	 * being properly restored.
	 */
	if (flags & IF_FLAG)
		osenv_intr_enable();
	else
		osenv_intr_disable();
}

