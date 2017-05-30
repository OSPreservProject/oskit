/*
 * Copyright (c) 2000 The University of Utah and the Flux Group.
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
#include <linux/kernel.h>
#include <linux/config.h>

#ifdef CONFIG_BLK_DEV_FD
/*
 * Stub to avoid needing to drag in Linux device drivers just for FS.
 * Used by FS code to support booting from floppy.
 *
 * Split off from wait_for_keypress since that glue routine is always needed
 * while this one is only needed if the linux device drivers are NOT used.
 */
void
floppy_eject(void)
{
	extern void wait_for_keypress(void);

	printk("Remove floppy from drive and then press return\n");
	wait_for_keypress();
	printk("Thanks!\n");
}
#endif
