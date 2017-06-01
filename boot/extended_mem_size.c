/*
 * Copyright (c) 1998 The University of Utah and the Flux Group.
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

#include <oskit/x86/pc/rtc.h>
#include <oskit/x86/proc_reg.h>
#include <oskit/c/assert.h>

#include "boot.h"

/*
 * Returns size of extended memory in kilobytes.
 * Extended memory is defined to start at the 1M boundary.
 *
 * First ask the Real Time Clock.
 * It can only tell us the truth up to 64M,
 * although some max out at 63M.
 * So we only believe it if it tells us less than 60M.
 *
 * Otherwise we probe by writing into pages starting where the RTC told us
 * and stopping when they won't store.
 * I.e. stopping at the first hole,
 * which is usually the end of memory.
 *
 * XXX this assumes that the first memory hole is on a 4K boundary.
 * This should be a pretty safe assumption.
 */
oskit_size_t
extended_mem_size()
{
	extern unsigned long _end[];
	oskit_size_t memsize;
	volatile unsigned long *addr;
	unsigned int old_cr0;

	/*
	 * Try the RTC.
	 */
	memsize = (rtcin(RTC_DEXTHI) << 8) | rtcin(RTC_DEXTLO);	/* in Kb */
	if (memsize < 60*1024)
		return memsize;

	/*
	 * Try probing memory starting where the RTC reported,
	 * rounded down to a 4K boundary because the RTC will max at 2^16 - 1.
	 */
	memsize &= ~3;			/* round down to 4K boundary */

	/* Turn off caching. */
	asm volatile("wbinvd");
	old_cr0 = get_cr0();
	set_cr0(old_cr0 | CR0_CD | CR0_NW);

	/* Try addrs at 4K intervals until they don't store. */
	addr = (unsigned long *)(memsize*1024 + 1024*1024);
	assert(_end <= addr);		/* avoid exotic binary tatoos */
	while (1) {
		*addr = 0x55555555;	/* 0101... */
		if (*addr != 0x55555555)
			break;

		*addr = 0xaaaaaaaa;	/* 1010... */
		if (*addr != 0xaaaaaaaa)
			break;

		*addr = 0xffffffff;	/* 1111... */
		if (*addr != 0xffffffff)
			break;

		*addr = 0x0;		/* 0000... */
		if (*addr != 0x0)
			break;

		addr = (unsigned long *)((oskit_size_t)addr + 4096);
	}
	memsize = ((oskit_size_t)addr - 1024*1024) / 1024;

	/* Turn caching back on. */
	set_cr0(old_cr0);

	return memsize;
}

