/*
 * Copyright (c) 1996, 1998 University of Utah and the Flux Group.
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
 * CPU identification code for x86 processors.
 */

#include <stdlib.h>
#include <oskit/x86/eflags.h>
#include <oskit/x86/proc_reg.h>
#include <oskit/x86/cpuid.h>
#include <oskit/c/string.h>

static void get_cache_config(struct cpu_info *id)
{
	unsigned ci[4];
	unsigned cicount = 0;
	unsigned ccidx = 0;

	do
	{
		unsigned i;

		cicount++;
		asm volatile("
			cpuid
		" : "=a" (ci[0]), 
		    "=b" (ci[1]),
		    "=c" (ci[2]),
		    "=d" (ci[3])
		  : "a" (2));

		for (i = 0; i < 4; i++)
		{
			unsigned reg = ci[i];
			if ((reg & (1 << 31)) == 0)
			{
				/* The low byte of EAX isn't a descriptor.  */
				if (i == 0)
					reg >>= 8;
				while (reg != 0)
				{
					if ((reg & 0xff) &&
					    (ccidx < sizeof(id->cache_config)))
					{
						id->cache_config[ccidx++] =
							reg & 0xff;
					}
					reg >>= 8;
				}
			}
		}
	}
	while (cicount < (ci[0] & 0xff));
}

void cpuid(struct cpu_info *out_id)
{
	int orig_eflags = get_eflags();

	memset(out_id, 0, sizeof(*out_id));

	/* Check for a dumb old 386 by trying to toggle the AC flag.  */
	set_eflags(orig_eflags ^ EFL_AC);
	if ((get_eflags() ^ orig_eflags) & EFL_AC)
	{
		/* It's a 486 or better.  Now try toggling the ID flag.  */
		set_eflags(orig_eflags ^ EFL_ID);
		if ((get_eflags() ^ orig_eflags) & EFL_ID)
		{
			int highest_val;

			/*
			 * CPUID is supported, so use it.
			 * First get the vendor ID string.
			 */
			asm volatile("
				cpuid
			" : "=a" (highest_val),
			    "=b" (*((int*)(out_id->vendor_id+0))),
			    "=d" (*((int*)(out_id->vendor_id+4))),
			    "=c" (*((int*)(out_id->vendor_id+8)))
			  : "a" (0));

			/* Now the feature information.  */
			if (highest_val >= 1)
			{
				asm volatile("
					cpuid
				" : "=a" (*((int*)out_id)),
				    "=d" (out_id->feature_flags)
				  : "a" (1)
				  : "ebx", "ecx");
			}

			/* Cache and TLB information.  */
			if (highest_val >= 2)
				get_cache_config(out_id);
		}
		else
		{
			/* No CPUID support - it's an older 486.  */
			out_id->family = CPU_FAMILY_486;

			/* XXX detect FPU */
		}
	}
	else
	{
		out_id->family = CPU_FAMILY_386;

		/* XXX detect FPU */
	}

	set_eflags(orig_eflags);
}

