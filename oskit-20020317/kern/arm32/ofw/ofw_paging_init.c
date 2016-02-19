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
#include <stdlib.h>
#include <stdio.h>

#if 0
#define DPRINTF(fmt, args... ) kprintf(__FUNCTION__  ":" fmt , ## args)
#else
#define DPRINTF(fmt, args... )
#endif

void
ofw_paging_init(oskit_addr_t pdir_pa)
{
	int		chosen, mmu_phandle, mmu_ihandle, mem_phandle;
	int		len, actual, count, i;
	unsigned int	mapping, iomapping;
	struct ofw_translation *ptrans, *ptmp;
	struct ofw_memregion   *pphys, *pmem;

	mapping   = PT_AP(ARM32_AP_KRWURW) | ARM32_PTE_TYPE_SPAGE |
		  ARM32_PTE_CACHEABLE | ARM32_PTE_BUFFERABLE;
	iomapping = PT_AP(ARM32_AP_KRWURW) | ARM32_PTE_TYPE_SPAGE;

	/*
	 * Ask the OFW to dump its page translations.
	 */
	if ((chosen = OF_finddevice("/chosen")) == -1 ||
	    OF_getprop(chosen, "mmu", &mmu_ihandle, sizeof(int)) < 0)
		panic("ofw_paging_init: Could not get the handle");

	mmu_ihandle = OF_decode_int((unsigned char *)&mmu_ihandle);

	if ((mmu_phandle = OF_instance_to_package(mmu_ihandle)) == -1)
		panic("ofw_paging_init: Could get MMU phandle");
	if ((len = OF_getproplen(mmu_phandle, "translations")) <= 0)
		panic("ofw_paging_init: OF_getproplen(trans)");

	/* Fluff in case OFW adds translations during this routine */
	len += (4 * sizeof(struct ofw_translation));

	if ((ptrans = (struct ofw_translation *) alloca(len)) == NULL)
		panic("ofw_paging_init: alloca()");

	if ((actual =
	     OF_getprop(mmu_phandle, "translations", ptrans, len)) > len)
		panic("ofw_paging_init: OF_getprop(translations)");

	count = actual / sizeof(struct ofw_translation);

	/*
	 * Finally! Okay, go through and add the OFWs translations
	 * to our own table.
	 *
	 * Note that we duplicate the OFWs code/data regions so that
	 * we can pass control back at reboot.
	 */
	for (i = 0, ptmp = ptrans; i < count; i++, ptmp++) {
		ptmp->virt = OF_decode_int((unsigned char *)&ptmp->virt);
		ptmp->size = OF_decode_int((unsigned char *)&ptmp->size);
		ptmp->phys = OF_decode_int((unsigned char *)&ptmp->phys);
		ptmp->mode = OF_decode_int((unsigned char *)&ptmp->mode);

		DPRINTF("VIRT->PHYS: "
			"virt:0x%x phys:0x%x size:0x%x mode:0x%x\n",
			ptmp->virt, ptmp->phys, ptmp->size, ptmp->mode);

		/*
		 * Maintain cacheable/bufferable bits.
		 */
		if (ptmp->mode & 0xC)
			pdir_map_range(pdir_pa,
				       ptmp->virt, ptmp->phys, ptmp->size,
				       mapping);
		else
			pdir_map_range(pdir_pa,
				       ptmp->virt, ptmp->phys, ptmp->size,
				       iomapping);
	}

	/*
	 * Get info about all memory, and equiv map all of physical memory.
	 */
	if ((mem_phandle = OF_finddevice("/memory")) == -1)
		panic("ofw_paging_init: OF_finddevice(/memory)");
	if ((len = OF_getproplen(mem_phandle, "reg")) <= 0)
		panic("ofw_paging_init: OF_getproplen(reg)");

	if ((pphys = (struct ofw_memregion *) alloca(len)) == NULL)
		panic("ofw_paging_init: alloca()");
	
	if (OF_getprop(mem_phandle, "reg", pphys, len) != len)
		panic("ofw_paging_init: OF_getprop(reg)");

	count = len / sizeof(struct ofw_memregion);

	for (i = 0, pmem = pphys; i < count; i++, pmem++) {
		pmem->start = OF_decode_int((unsigned char *)&pmem->start);
		pmem->size = OF_decode_int((unsigned char *)&pmem->size);

		DPRINTF("VIRT->PHYS: "
			"virt:0x%x phys:0x%x size:0x%x mode:0x%x\n",
			pmem->start, pmem->start, pmem->size, mapping);

		pdir_map_range(pdir_pa,
			       pmem->start, pmem->start, pmem->size, mapping);
	}
}

