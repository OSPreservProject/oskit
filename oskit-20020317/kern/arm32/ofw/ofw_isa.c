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
#define DPRINTF(fmt, args... ) kprintf(__FUNCTION__  ":" fmt , ## args)
#else
#define DPRINTF(fmt, args... )
#endif

/*
 * For outb/inb style access.
 */
oskit_addr_t		isa_iobus_address;
oskit_addr_t		isa_membus_address;

/*
 * This structure describes the address range of a bus.
 */
struct vl_range {
	oskit_addr_t	vl_phys_hi;
	oskit_addr_t	vl_phys_lo;
	oskit_addr_t	parent_phys_start;
	oskit_size_t	vl_size;
};

struct vl_isa_range {
	oskit_addr_t	isa_phys_hi;
	oskit_addr_t	isa_phys_lo;
	oskit_addr_t	parent_phys_hi;
	oskit_addr_t	parent_phys_lo;
	oskit_size_t	isa_size;
};

struct dma_range {
	oskit_addr_t	start;
	oskit_size_t	size;
};

/*
 * Configure the ISA bus. Ask the OFW for the physical memory addresses
 * of the I/O and Mem regions. Then map the regions into the address space
 * using the ofw map routines. When complete, the ISA will be accessible
 * in the virtual memory space.
 *
 * XXX: This code assumes that the only VL ranges are for the ISA. This
 * might need to be more intelligent on machines with more than an ISA.
 */
void
ofw_configisamem(void)
{
	int		vl, i;
	oskit_addr_t	pio = 0, pmem = 0;
	struct vl_range vl_ranges[2];

	if ((vl = OF_finddevice("/vlbus")) == -1)
		panic("ofw_configisa: OF_finddevice(/vlbus)");

	if (OF_getprop(vl, "ranges", vl_ranges, sizeof(vl_ranges))
	    != sizeof(vl_ranges))
		panic("ofw_configisa: OF_getprop(vl ranges)");

	/*
	 * Decode the ranges. It appears that the low bit of the phys_hi
	 * address indicates I/O or Mem.
	 */
	for(i = 0; i < 2; i++) {
		int hi = OF_decode_int((unsigned char *)
				       &(vl_ranges[i].vl_phys_hi));

		DPRINTF("0x%x 0x%x 0x%x %d\n",
			OF_decode_int((unsigned char *)
				      &(vl_ranges[i].vl_phys_hi)),
			OF_decode_int((unsigned char *)
				      &(vl_ranges[i].vl_phys_lo)),
			OF_decode_int((unsigned char *)
				      &(vl_ranges[i].parent_phys_start)),
			OF_decode_int((unsigned char *)
				      &(vl_ranges[i].vl_size)));

		if (hi & 1)
			pio  = (oskit_addr_t) OF_decode_int((unsigned char *)
					&(vl_ranges[i].parent_phys_start));
		else
			pmem = (oskit_addr_t) OF_decode_int((unsigned char *)
					&(vl_ranges[i].parent_phys_start));
	}
	DPRINTF("pio:0x%x, pmem:0x%x\n", pio, pmem);

	/*
	 * Now map them into the virtual address space of the kernel so
	 * the ISA can be accessed.
	 */
	ofw_iomap(ISA_IOB_VIRTADDR, pio,  0x100000, 0);
	ofw_iomap(ISA_MEM_VIRTADDR, pmem, 0x100000, 0);

	/*
	 * Indirect since we might run without the MMU someday.
	 */
	isa_membus_address = ISA_MEM_VIRTADDR;
	isa_iobus_address  = ISA_IOB_VIRTADDR;
}


void
ofw_configisadma(void)
{
	int		root, i, size, count;
	struct dma_range *ranges, *pdr;

	if ((root = OF_finddevice("/")) == -1)
		panic("ofw_configisadma: OF_finddevice(/)");

	if ((size = OF_getproplen(root, "dma-ranges")) <= 0)
		panic("ofw_configisadma: OF_getproplen(dma_ranges)");

	if ((ranges = (struct dma_range *) alloca(size)) == NULL)
		panic("ofw_configisadma: alloca()");

	if (OF_getprop(root, "dma-ranges", ranges, size) != size)
		panic("ofw_configisadma: OF_getprop()");

	count = size / sizeof(struct dma_range);

	DPRINTF("ofw_configisadma: %d %d\n", size, count);

	for (i = 0, pdr = ranges; i < count; i++, pdr++) {
		pdr->start = OF_decode_int((unsigned char *)&pdr->start);
		pdr->size  = OF_decode_int((unsigned char *)&pdr->size);

		DPRINTF("DMA Range: 0x%08x 0x%08x\n", pdr->start, pdr->size);
	}
}

void
ofw_configisa(void)
{
	ofw_configisamem();
	ofw_configisadma();
}

