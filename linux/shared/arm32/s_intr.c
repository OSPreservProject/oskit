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

#include <oskit/machine/proc_reg.h>
#include "osenv.h"

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

	rc = get_cpsr();

	if (osenv_intr_enabled())
		rc &= ~(PSR_I);
	else
		rc |= (PSR_I);
		
	return rc;
}

void
linux_restore_flags(unsigned flags)
{
	/*
	 * XXX: Linux drivers only count on the interrupt enable bit
	 * being properly restored.
	 */
	if (flags & PSR_I)
		osenv_intr_disable();
	else
		osenv_intr_enable();
}

unsigned
linux_save_flags_cli()
{
	unsigned rc;

	rc = get_cpsr();

	if (osenv_intr_save_disable())
		rc &= ~(PSR_I);
	else
		rc |= (PSR_I);
		
	return rc;
}

