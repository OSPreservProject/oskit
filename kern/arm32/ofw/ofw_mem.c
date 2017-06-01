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
 * Talk to the OFW about memory stuff. 
 */
#include <oskit/types.h>
#include <oskit/machine/ofw/ofw.h>
#include <stdlib.h>
#include <stdio.h>
#include <alloca.h>

#if 0
#define DPRINTF(fmt, args... ) kprintf(__FUNCTION__  ":" fmt , ## args)
#else
#define DPRINTF(fmt, args... )
#endif

/*
 * Ask the OFW about physical memory. The OFW returns two arrays, one that
 * describes all of physical memory, and a second that describes all of the
 * physical memory that the OFW has not claimed yet. 
 */
void
ofw_getphysmeminfo(void)
{
	int	phandle;
	int	i, len, physmem_count, availmem_count;
	struct ofw_memregion	physmem[32];		/* XXX No malloc yet */
	struct ofw_memregion	availmem[32];		/* XXX No malloc yet */
	struct ofw_memregion	*pmem;

	/*
	 * Get info about all memory.
	 */
	if ((phandle = OF_finddevice("/memory")) == -1)
		panic("ofw_getphysmeminfo: OF_finddevice(/memory)");
	if ((len = OF_getproplen(phandle, "reg")) <= 0)
		panic("ofw_getphysmeminfo: OF_getproplen(reg)");
	if (OF_getprop(phandle, "reg", physmem, len) != len)
		panic("ofw_getphysmeminfo: OF_getprop(reg)");

	if ((physmem_count = len / sizeof(struct ofw_memregion)) >= 32)
		panic("ofw_getphysmeminfo: Too many physmem regions\n");

	for (i = 0, pmem = physmem; i < physmem_count; i++, pmem++) {
		pmem->start = OF_decode_int((unsigned char *)&pmem->start);
		pmem->size = OF_decode_int((unsigned char *)&pmem->size);

		DPRINTF("region %d: 0x%x %d\n", i, pmem->start, pmem->size);
	}

	/*
	 * Get info about available memory (not yet claimed by the OFW).
	 */
	if ((len = OF_getproplen(phandle, "available")) <= 0)
		panic("ofw_getphysmeminfo: OF_getproplen(available)");
	if (OF_getprop(phandle, "available", availmem, len) != len)
		panic("ofw_getphysmeminfo: OF_getprop(available)");

	if ((availmem_count = len / sizeof(struct ofw_memregion)) >= 32)
		panic("ofw_getphysmeminfo: Too many availmem regions\n");

	for (i = 0, pmem = availmem; i < availmem_count; i++, pmem++) {
		pmem->start = OF_decode_int((unsigned char *)&pmem->start);
		pmem->size = OF_decode_int((unsigned char *)&pmem->size);

		DPRINTF("region %d: 0x%x %d\n", i, pmem->start, pmem->size);
	}
}

