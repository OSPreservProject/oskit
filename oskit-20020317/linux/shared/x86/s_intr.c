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

unsigned
linux_save_flags_cli()
{
	unsigned rc;

	asm volatile("
		pushfl
		popl %0" : "=r" (rc));

	rc &= ~IF_FLAG;
	if (osenv_intr_save_disable())
		rc |= IF_FLAG;

	return rc;
}
