/*
 * Copyright (c) 1997-2000 University of Utah and the Flux Group.
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
 * start_blk_devices.c
 *
 * start up the block devices.
 */

#include <oskit/startup.h>
#include <oskit/dev/linux.h>

void
start_blk_devices(void)
{
	start_devices();
}

/*
 * Having this initialization here makes `start_devices' (start_devices.c)
 * initialize and probe the disk drivers, since linking in this file
 * means the program is using the disk.
 */
#ifdef HAVE_CONSTRUCTOR
/*
 * Place initilization in the init section to be called at startup
 */
static void initme(void) __attribute__ ((constructor));

static void
initme(void)
{
	extern void (*_init_devices_blk)(void);

	_init_devices_blk = oskit_linux_init_blk;
}
#else
/*
 * Use a (dreaded) common symbol
 */
void (*_init_devices_blk)(void) = oskit_linux_init_blk;
#endif
