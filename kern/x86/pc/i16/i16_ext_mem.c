/*
 * Copyright (c) 1994-1995, 1998 University of Utah and the Flux Group.
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
#if 0

#include <oskit/x86/i16.h>
#include <oskit/x86/types.h>
#include <oskit/x86/far_ptr.h>
#include <oskit/x86/proc_reg.h>
#include <oskit/x86/pc/i16_bios.h>
#include <oskit/debug.h>

static oskit_addr_t ext_mem_phys_free_mem;
static oskit_size_t ext_mem_phys_free_size;


CODE32

int ext_mem_collect(void)
{
	if (ext_mem_phys_free_mem)
	{
		phys_mem_add(ext_mem_phys_free_mem, ext_mem_phys_free_size);
		ext_mem_phys_free_mem = 0;
	}
}

CODE16

void i16_ext_mem_check()
{
	oskit_addr_t ext_mem_top, ext_mem_bot;
	unsigned short ext_mem_k;

	/* Find the top of available extended memory.  */
	asm volatile("
		int	$0x15
		jnc	1f
		xorw	%%ax,%%ax
	1:
	" : "=a" (ext_mem_k)
	  : "a" (0x8800));
	ext_mem_top = 0x100000 + (oskit_addr_t)ext_mem_k * 1024;

	/* XXX check for >16MB memory using function 0xc7 */

	ext_mem_bot = 0x100000;

	/* Check for extended memory allocated bottom-up: method 1.
	   This uses the technique (and, loosely, the code)
	   described in the VCPI spec, version 1.0.  */
	if (ext_mem_top > ext_mem_bot)
	{
		asm volatile("
			pushw	%%es

			xorw	%%ax,%%ax
			movw	%%ax,%%es
			movw	%%es:0x19*4+2,%%ax
			movw	%%ax,%%es

			movw	$0x12,%%di
			movw	$7,%%cx
			rep
			cmpsb
			jne	1f

			xorl	%%edx,%%edx
			movb	%%es:0x2e,%%dl
			shll	$16,%%edx
			movw	%%es:0x2c,%%dx

		1:
			popw	%%es
		" : "=d" (ext_mem_bot)
		  : "d" (ext_mem_bot),
		    "S" ((unsigned short)(oskit_addr_t)"VDISK V")
		  : "eax", "ecx", "esi", "edi");
	}
	i16_assert(ext_mem_bot >= 0x100000);

	/* Check for extended memory allocated bottom-up: method 2.
	   This uses the technique (and, loosely, the code)
	   described in the VCPI spec, version 1.0.  */
	if (ext_mem_top > ext_mem_bot)
	{
		struct {
			char pad1[3];
			char V;
			long DISK;
			char pad2[30-8];
			unsigned short addr;
		} buf;
		unsigned char rc;

		i16_assert(sizeof(buf) == 0x20);
		rc = i16_bios_copy_ext_mem(0x100000, kvtolin((oskit_addr_t)&buf), sizeof(buf)/2);
		if ((rc == 0) && (buf.V == 'V') && (buf.DISK == 'DISK'))
		{
			oskit_addr_t new_bot = (oskit_addr_t)buf.addr << 10;
			i16_assert(new_bot > 0x100000);
			if (new_bot > ext_mem_bot)
				ext_mem_bot = new_bot;
		}
	}
	i16_assert(ext_mem_bot >= 0x100000);

	if (ext_mem_top > ext_mem_bot)
	{
		ext_mem_phys_free_mem = ext_mem_bot;
		ext_mem_phys_free_size = ext_mem_top - ext_mem_bot;

		/* We need to update phys_mem_max here
		   instead of just letting phys_mem_add() do it
		   when the memory is collected with phys_mem_collect(),
		   because VCPI initialization needs to know the top of physical memory
		   before phys_mem_collect() is called.
		   See i16_vcpi.c for the gross details.  */
		if (ext_mem_top > phys_mem_max)
			phys_mem_max = ext_mem_top;
	}
}

void i16_ext_mem_shutdown()
{
	/* We didn't actually allocate the memory,
	   so no need to deallocate it... */
}

#endif
