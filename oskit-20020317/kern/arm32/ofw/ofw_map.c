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
#include <oskit/machine/paging.h>
#include <stdlib.h>
#include <stdio.h>

#if 0
#define DPRINTF(fmt, args... ) kprintf(__FUNCTION__  ":" fmt , ## args)
#else
#define DPRINTF(fmt, args... )
#endif

/* forward decls */
static int	ofw_mmu_ihandle(void);
static void	ofw_claimvirt(oskit_addr_t, oskit_size_t);
static void	ofw_claimphys(oskit_addr_t, oskit_size_t);
static void	ofw_settranslation(oskit_addr_t, oskit_addr_t,
				oskit_size_t, oskit_u32_t);

/*
 * Create a virtual to I/O memory translation using the OFW.
 * First "claim" the virtual memory by asking the OFW for it. Then
 * ask the OFW to add the translation to its pages tables.
 */
void
ofw_iomap(oskit_addr_t va,
	  oskit_addr_t pa, oskit_size_t size, oskit_u32_t prot)
{
	DPRINTF("va:0x%x pa:0x%x size:%d\n", va, pa, size);
	ofw_claimvirt(va, size);
	ofw_settranslation(va, pa, size,
			   (ARM32_AP_KRW << 10) | prot);
}

/*
 * This is very much the same as iomap.
 */
void
ofw_physmap(oskit_addr_t pa, oskit_size_t size, oskit_u32_t prot)
{
	DPRINTF("pa:0x%x size:%d\n", pa, size);
	ofw_settranslation(pa, pa, size,
			   (ARM32_AP_KRWURW << 10) |
			   ARM32_PTE_BUFFERABLE|ARM32_PTE_CACHEABLE);
}

/*
 * Create a virtual to physical memory translation using the OFW.
 * Must "claim" both the physical *and* virtual memory by asking the OFW
 * for it. Then* ask the OFW to add the translation to its pages tables.
 */
void
ofw_vmmap(oskit_addr_t va,
	  oskit_addr_t pa, oskit_size_t size, oskit_u32_t prot)
{
	DPRINTF("va:0x%x pa:0x%x size:%d\n", va, pa, size);
	ofw_claimvirt(va, size);
	ofw_claimphys(pa, size);
	ofw_settranslation(va, pa, size,
			   (ARM32_AP_KRWURW << 10) |
			   ARM32_PTE_BUFFERABLE|ARM32_PTE_CACHEABLE);
}

/*
 * Tell the OFW to unmap memory.
 */
void
ofw_unmap(oskit_addr_t va, oskit_size_t size)
{
	int	     mmu_ihandle = ofw_mmu_ihandle();

	DPRINTF("va:0x%x size:%d\n", va, size);

	if ((OF_call_method("unmap", mmu_ihandle, 2, 0, va, size)) != 0)
		panic("ofw_unmap: unmap failed a:0x%x size:%d\n", va, size);
}

/*
 * Allocate a specific block of virtual memory from the OFW. If it fails
 * just panic. No alignment allowed; set to zero.
 */
static void
ofw_claimvirt(oskit_addr_t va, oskit_size_t size)
{
	int	     mmu_ihandle = ofw_mmu_ihandle();
	oskit_addr_t result_va;

	DPRINTF("va:0x%x size:%d\n", va, size);

	result_va = OF_call_method_1("claim", mmu_ihandle, 3, va, size, 0);
	if (result_va != va)
		panic("ofw_claimvirt: Could not allocate va:0x%x\n", va);
}

/*
 * Claimphys is really the same thing as claimvirt! Tell the OFW to give
 * us physical memory.
 */
static void
ofw_claimphys(oskit_addr_t pa, oskit_size_t size)
{
	int	     mmu_ihandle = ofw_mmu_ihandle();
	oskit_addr_t result_pa;

	DPRINTF("pa:0x%x size:%d\n", pa, size);

	result_pa = OF_call_method_1("claim", mmu_ihandle, 3, pa, size, 0);
	if (result_pa != pa)
		panic("ofw_claimphys: Could not allocate pa:0x%x\n", pa);
}

/*
 * Tell the OFW to map va->pa.
 */
static void
ofw_settranslation(oskit_addr_t va, oskit_addr_t pa,
		   oskit_size_t size, oskit_u32_t mode)
{
	int mmu_ihandle = ofw_mmu_ihandle();

	DPRINTF("va:0x%x pa:0x%x size:%d prot:0x%x\n", va, pa, size, mode);
	
	if (OF_call_method("map", mmu_ihandle, 4, 0, pa, va, size, mode) != 0)
		panic("ofw_settranslation: Map failed");
}

/*
 * Ask the OFW for a translation
 */
oskit_addr_t
ofw_gettranslation(oskit_addr_t va)
{
	int		mmu_ihandle = ofw_mmu_ihandle();
	oskit_addr_t	pa;
	int		mode;
	int		exists;

	exists = 0;
	
	if (OF_call_method("translate",
			   mmu_ihandle, 1, 3, va, &pa, &mode, &exists) != 0)
		return(-1);

	return(exists ? pa : -1);
}

/*
 * Stash the "mmu" OFW handle.
 */
static int
ofw_mmu_ihandle(void)
{
	static int mmu_ihandle = 0;
	int chosen;

	if (mmu_ihandle != 0)
		return(mmu_ihandle);

	if ((chosen = OF_finddevice("/chosen")) == -1 ||
	    OF_getprop(chosen, "mmu", &mmu_ihandle, sizeof(int)) < 0)
		panic("ofw_mmu_ihandle: Could not get the handle");

	mmu_ihandle = OF_decode_int((unsigned char *)&mmu_ihandle);

	return(mmu_ihandle);
}

/*
 * Ask the OFW about virtual memory.
 */
void
ofw_getvirtualtranslations(void)
{
	int		mmu_phandle, mmu_ihandle;
	int		len, actual, count, i;
	struct ofw_translation *ptrans, *ptmp;

	mmu_ihandle = ofw_mmu_ihandle();

	if ((mmu_phandle = OF_instance_to_package(mmu_ihandle)) == -1)
		panic("ofw_getvirtualtranslations: Could get MMU phandle");
	if ((len = OF_getproplen(mmu_phandle, "translations")) <= 0)
		panic("ofw_getvirtualtranslations: OF_getproplen(trans)");

	/* Fluff in case OFW adds translations during this routine */
	len += (4 * sizeof(struct ofw_translation));

	if ((ptrans = (struct ofw_translation *) alloca(len)) == NULL)
		panic("ofw_getvirtualtranslations: alloca()");

	if ((actual =
	     OF_getprop(mmu_phandle, "translations", ptrans, len)) > len)
		panic("ofw_getvirtualtranslations: OF_getprop(translations)");

	count = actual / sizeof(struct ofw_translation);

	for (i = 0, ptmp = ptrans; i < count; i++, ptmp++) {
		ptmp->virt = OF_decode_int((unsigned char *)&ptmp->virt);
		ptmp->size = OF_decode_int((unsigned char *)&ptmp->size);
		ptmp->phys = OF_decode_int((unsigned char *)&ptmp->phys);
		ptmp->mode = OF_decode_int((unsigned char *)&ptmp->mode);

		DPRINTF("VIRT->PHYS: "
			"virt:0x%x phys:0x%x size:0x%x mode:0x%x\n",
			ptmp->virt, ptmp->phys, ptmp->size, ptmp->mode);
	}
}

