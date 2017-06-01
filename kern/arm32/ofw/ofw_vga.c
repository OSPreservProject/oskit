/*
 * Copyright (c) 1999 University of Utah and the Flux Group.
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
 * Copyright 1997
 * Digital Equipment Corporation. All rights reserved.
 *
 * This software is furnished under license and may be used and
 * copied only in accordance with the following terms and conditions.
 * Subject to these conditions, you may download, copy, install,
 * use, modify and distribute this software in source and/or binary
 * form. No title or ownership is transferred hereby.
 *
 * 1) Any source code used, modified or distributed must reproduce
 *    and retain this copyright notice and list of conditions as
 *    they appear in the source file.
 *
 * 2) No right is granted to use any trade name, trademark, or logo of
 *    Digital Equipment Corporation. Neither the "Digital Equipment
 *    Corporation" name nor any trademark or logo of Digital Equipment
 *    Corporation may be used to endorse or promote products derived
 *    from this software without the prior written permission of
 *    Digital Equipment Corporation.
 *
 * 3) This software is provided "AS-IS" and any express or implied
 *    warranties, including but not limited to, any implied warranties
 *    of merchantability, fitness for a particular purpose, or
 *    non-infringement are disclaimed. In no event shall DIGITAL be
 *    liable for any damages whatsoever, and in particular, DIGITAL
 *    shall not be liable for special, indirect, consequential, or
 *    incidental damages or damages for lost profits, loss of
 *    revenue or loss of use, whether such damages arise in contract,
 *    negligence, tort, under statute, in equity, at law or otherwise,
 *    even if advised of the possibility of such damage.
 */

/*
 * Talk to the OFW about ISA bus stuff. 
 */
#include <oskit/types.h>
#include <oskit/machine/ofw/ofw.h>
#include <oskit/machine/shark/isa.h>
#include <stdlib.h>
#include <stdio.h>

#if 0
#define DPRINTF(fmt, args... ) printf(__FUNCTION__  ":" fmt , ## args)
#else
#define DPRINTF(fmt, args... )
#endif

/*
 * This puts the CT65550 into VGA mode so that we can implement a trivial
 * console interface.
 */
int
ofw_vga_init(void)
{
	int	root;
	int	ihandle, phandle;
	char	buf[128];

	/*
	 * Find out whether the firmware's chosen stdout is a display.
	 * If so, use the existing ihandle so the firmware doesn't
	 * become unhappy.  If not, just open it.
	 */
	if ((root = OF_finddevice("/chosen")) == -1)
		panic("ofw_vga_init: OF_finddevice(/choosen)");

	if (OF_getprop(root, "stdout", &ihandle, sizeof(ihandle)) !=
	    sizeof(ihandle))
		panic("ofw_vga_init: OF_getprop(stdout)");

	ihandle = OF_decode_int((unsigned char *)&ihandle);
	
	if ((phandle = OF_instance_to_package(ihandle)) == -1)
		panic("ofw_vga_init: OF_instance_to_package(ihandle)");

	if (OF_getprop(phandle, "device_type", buf, sizeof(buf)) <= 0)
		panic("ofw_vga_init: OF_getprop(device_type)");

	DPRINTF("Device Type: %s\n", buf);

	if (strcmp(buf, "display")) {
		/*
		 * The display is not the standard output device.
		 * Must open it.
		 */
		if ((ihandle = OF_open("screen")) == 0)
			panic("ofw_vga_init: OF_open(screen)");
	}

	/*
	 * Okay, now force the display into VGA mode.
	 */
	if (OF_call_method("text-mode3", ihandle, 0, 0) != 0)
		panic("ofw_vga_init: OF_call_method(text-mode3)");

	return 0;
}
