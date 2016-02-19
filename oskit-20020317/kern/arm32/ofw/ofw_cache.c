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
 * Talk to the OFW about memory mapping stuff. 
 */
#include <oskit/types.h>
#include <oskit/machine/ofw/ofw.h>
#include <oskit/machine/base_paging.h>
#include <oskit/machine/base_vm.h>
#include <stdlib.h>
#include <stdio.h>

#if 0
#define DPRINTF(fmt, args... ) kprintf(__FUNCTION__  ":" fmt , ## args)
#else
#define DPRINTF(fmt, args... )
#endif

oskit_addr_t
ofw_getcleaninfo(void)
{
	int		cpu;
	oskit_addr_t	vclean, pclean;

	if ((cpu = OF_finddevice("/cpu")) == -1)
		panic("ofw_getcleaninfo: OF_finddevice(/cpu)");
	
	if ((OF_getprop(cpu, "d-cache-flush-address", &vclean,
			sizeof(vclean))) != sizeof(vclean))
		return -1;

	vclean = OF_decode_int((unsigned char *)&vclean);

	if ((pclean = ofw_gettranslation(vclean)) == -1)
		panic("ofw_getcleaninfo: ofw_gettranslation(0x%x)", vclean);

	return pclean;
}

void
ofw_cacheclean_init(void)
{
	/*
	 * Get the physical address of the clean area.
	 */
	if ((arm32_cache_clean_addr = ofw_getcleaninfo()) == -1)
		panic("ofw_paging_init: Could not get clean info");
	arm32_cache_clean_size = 0x4000;

	/*
	 * Create an ofw equiv mapping for it. It must be cacheable or
	 * bufferable, which is what makes it the "clean" space.
	 */
	ofw_physmap(arm32_cache_clean_addr, arm32_cache_clean_size, 0);

	DPRINTF("VIRT->PHYS: "
		"virt:0x%x phys:0x%x size:0x%x\n",
		arm32_cache_clean_addr, arm32_cache_clean_addr,
		arm32_cache_clean_size);
}
