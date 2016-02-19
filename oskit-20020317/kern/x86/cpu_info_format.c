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
 * Routine to pretty-print x86 CPU identification information.
 */

#include <stdio.h>
#include <string.h>

#include <oskit/x86/cpuid.h>

void cpu_info_format(
	struct cpu_info *id,
	void (*formatter)(void *data, const char *fmt, ...),
	void *data)
{
	static char *cputype[4] = {"0 (Original OEM processor)",
				   "1 (OverDrive processor)",
				   "2 (Dual processor)",
				   "3 (Intel Reserved)"};
	static const char *features[] = {
		"On-chip FPU",
		"Virtual 8086 mode enhancement",
		"I/O breakpoints",
		"4MB page size extensions",
		"Time stamp counter",
		"Model-specific registers",
		"36-bit physical address extension",
		"Machine check exception",
		"CMPXCHG8B instruction",
		"On-chip local APIC",
		0,
/*
 * XXX: Intel says that FAST_SYSCALL is not supported on family==6,
 * model < 3, and stepping < 3, regardless of the flag.
 */
		"Fast system call (maybe)",
		"Memory type range registers",
		"Global bit in page table entries",
		"Machine check architecture",
		"CMOVcc instruction",
		"Page attribute table",
		"36-bit page size extension",
		"Processor Serial Number",
		0,
		0,
		0,
		0,
		"MMX instructions",
		"Fast FP save/restore",
		"SSE instructions",
		0,
		0,
		0,
		0,
		"IA-64 architecture",
		0,		
		};
	const char *company = "";
	const char *modelname = "";
	char fam[4];
	const char *family = fam;
	char vendorname[13];
	unsigned i;

	/*
	 * First try to guess a descriptive processor name
	 * based on the vendor ID, family, and model information.
	 * Much of this information comes from the x86 info file
	 * compiled by Christian Ludloff, ludloff@anet-dfw.com,
	 * http://webusers.anet-dfw.com/~ludloff.
	 */

	/* Default family string: #86 */
	fam[0] = '0' + id->family;
	fam[1] = '8';
	fam[2] = '6';
	fam[3] = 0;

	/* Check for specific vendors and models */
	if (memcmp(id->vendor_id, "GenuineIntel", 12) == 0)
	{
		static const char *m486[] =
			{"DX", "DX", "SX", "DX/2 or 487", "SL",
			 "SX/2", "", "DX/2-WB", "DX/4"};
		static const char *mp5[] =
			{"",
			 " 60/66",	/* or overdrive for same */
			 " 75/90/100/120/133/150/166/200",
			 "",
			 " MMX"};
		static const char *mp5o[] =
			{"",
			 "", /* 60/66 Overdrive, in theory */
			 "", /* 75-133 Overdrive, in theory */
			 " Overdrive for 486",
			 " MMX Overdrive"};
		static const char *mp6[] =
			{"", " Pro", "", " II", "",
			 " II/Celeron/Xeon", " Celeron 300A/333",
			 " III", " III Coppermine"}; 

		static const char *mp6o[] =
			{"", "", "", " Pro Overdrive"};

		company = "Intel ";
		switch (id->family)
		{
		case 4:
			if (id->model < sizeof(m486)/sizeof(m486[0]))
				modelname = m486[id->model];
			break;
		case 5:
			family = "Pentium";
			if ((id->model < sizeof(mp5)/sizeof(mp5[0])) &&
			    (id->type == 0))
				modelname = mp5[id->model];
			if ((id->model < sizeof(mp5o)/sizeof(mp5o[0])) &&
			    (id->type == 1))
				modelname = mp5o[id->model];
			break;
		case 6:
			family = "Pentium (P6)";
			if ((id->model < sizeof(mp6)/sizeof(mp6[0])) &&
			    (id->type == 0))
				modelname = mp6[id->model];
			if ((id->model < sizeof(mp6o)/sizeof(mp6o[0])) &&
			    (id->type == 1))
				modelname = mp6o[id->model];
			break;
		}
	}
	else if (memcmp(id->vendor_id, "UMC UMC UMC ", 12) == 0)
	{
		static const char *u486[] = {"", " U5D", " U5S"};

		company = "UMC ";
		switch (id->family)
		{
		case 4:
			if (id->model < sizeof(u486)/sizeof(u486[0]))
				modelname = u486[id->model];
			break;
		}
	}
	else if (memcmp(id->vendor_id, "AuthenticAMD", 12) == 0)
	{
		static const char *a486[] =
			{"", "", "", "DX2", "", "", "", "DX2WB",
			 "DX4", "DX4WB", "", "", "", "", "X5WT", "X5WB"};
		static const char *ak5[] = {" SSA5", " 5k86"};

		company = "AMD ";
		switch (id->family)
		{
		case 4:
			if (id->model < sizeof(a486)/sizeof(a486[0]))
				modelname = a486[id->model];
			break;
		case 5:
			family = "K5";
			if (id->model < sizeof(ak5)/sizeof(ak5[0]))
				modelname = ak5[id->model];
			break;
		}
	}
	else if (memcmp(id->vendor_id, "CyrixInstead", 12) == 0)
	{
		company = "Cyrix ";
		switch (id->family)
		{
		case 4:
			family = "5x86";
			break;
		case 5:
			family = "6x86";
			break;
		}
	}
	else if (memcmp(id->vendor_id, "NexGenDriven", 12) == 0)
	{
		company = "NexGen ";
		switch (id->family)
		{
		case 5:
			family = "Nx586";
			break;
		}
	}
	formatter(data, "Processor: %s%s%s\n", company, family, modelname);

	/*
	 * Now print the specific information
	 * exactly as provided by the processor.
	 */
	memcpy(vendorname, id->vendor_id, 12);
	vendorname[12] = 0;
	formatter(data, "Vendor ID: %s\n", vendorname);
	formatter(data, "Family: %c86\n", '0' + id->family);
	formatter(data, "Model: %d\n", id->model);
	formatter(data, "Stepping: %c/%d\n", 'A' + id->stepping, id->stepping);
	formatter(data, "Type: %s\n", cputype[id->type]);

	/* Now the feature flags.  */
	for (i = 0; i < sizeof(features)/sizeof(features[0]); i++)
	{
		if (features[i])
			formatter(data, "%s: %s\n", features[i],
				  id->feature_flags & (1 << i) ? "yes" : "no");
	}

	/* Now the cache configuration information, if any.  */
	for (i = 0; i < sizeof(id->cache_config); i++)
	{
		char sbuf[40];
		const char *s = 0;

		switch (id->cache_config[i])
		{
		case 0: 
			break;
		case 0x01: s = "Instruction TLB: 4K-byte pages, "
				"4-way set associative, 64 entries"; break;
		case 0x02: s = "Instruction TLB: 4M-byte pages, "
				"fully associative, 2 entries"; break;
		case 0x03: s = "Data TLB: 4K-byte pages, "
				"4-way set associative, 64 entries"; break;
		case 0x04: s = "Data TLB: 4M-byte pages, "
				"4-way set associative, 8 entries"; break;
		case 0x06: s = "Instruction cache: 8K bytes, "
				"4-way set associative, 32 byte line size";
				break;
		case 0x08: s = "Instruction cache: 16K bytes, "
				"4-way set associative, 32 byte line size";
				break;
		case 0x0a: s = "Data cache: 8K bytes, "
				"2-way set associative, 32 byte line size";
				break;
		case 0x0c: s = "Data cache: 16K bytes, "
				"2-way set associative, 32 byte line size";
				break;
		case 0x40: s = "No L2 cache";
				break;
		case 0x41: s = "Unified cache: 128K bytes, "
				"4-way set associative, 32 byte line size";
				break;
		case 0x42: s = "Unified cache: 256K bytes, "
				"4-way set associative, 32 byte line size";
				break;
		case 0x43: s = "Unified cache: 512K bytes, "
				"4-way set associative, 32 byte line size";
				break;
		case 0x44: s = "Unified cache: 1024K bytes, "
				"4-way set associative, 32 byte line size";
				break;
		case 0x45: s = "Unified cache: 2048K bytes, "
				"4-way set associative, 32 byte line size";
				break;
	        case 0x82: s = "Unified cache: 256K bytes, "
			        "8-way set associative, 32 byte line size";
		default:
			sprintf(sbuf, "(unknown cache/TLB descriptor 0x%02x)",
				id->cache_config[i]);
			s = sbuf;
			break;
		}
		if (s)
			formatter(data, "%s\n", s);
	}
}

