/*
 * Copyright (c) 1996-1999 The University of Utah and the Flux Group.
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
#include <linux/errno.h>
#include <linux/mm.h>

int
verify_area(int rw, const void *p, unsigned long size)
{
	return (0);	/* XXX: need an fdev interface for this?  */
}

/*
 * Print device name (in decimal, hexadecimal or symbolic) -
 * at present hexadecimal only.
 * Note: returns pointer to static data!
 */
char *
kdevname(kdev_t dev)
{
	static char buffer[32];

	sprintf(buffer, "%02x:%02x", MAJOR(dev), MINOR(dev));
	return (buffer);
}
