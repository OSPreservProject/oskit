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
 * CPU identification for x86 processors.
 */
#ifndef _OSKIT_X86_CPUID_H_
#define _OSKIT_X86_CPUID_H_

#include <oskit/compiler.h>

struct cpu_info
{
	unsigned 	stepping	: 4;
	unsigned 	model		: 4;
	unsigned 	family		: 4;
	unsigned	type		: 2;
	unsigned	reserved	: 18;
	unsigned	feature_flags;
	char		vendor_id[12];
	unsigned char	cache_config[16];
};

/*
 * Values of the cpu_info.family field
 */
#define CPU_FAMILY_386		3
#define CPU_FAMILY_486		4
#define CPU_FAMILY_PENTIUM	5
#define CPU_FAMILY_PENTIUM_PRO	6

/*
 * Values of the cpu_info.type field
 */
#define CPU_TYPE_ORIGINAL	0
#define CPU_TYPE_OVERDRIVE	1
#define CPU_TYPE_DUAL		2

/*
 * CPU feature_flags bit definitions.
 */
#define CPUF_ON_CHIP_FPU	0x00000001	/* On-chip floating point */
#define CPUF_VM86_EXT		0x00000002	/* Virtual mode extensions */
#define CPUF_IO_BKPTS		0x00000004	/* I/O breakpoint support */
#define CPUF_4MB_PAGES		0x00000008	/* 4MB page support */
#define CPUF_TS_COUNTER		0x00000010	/* Timestamp counter */
#define CPUF_PENTIUM_MSR	0x00000020	/* Pentium model-spec regs */
#define CPUF_PAGE_ADDR_EXT	0x00000040	/* Page address extensions */
#define CPUF_MACHINE_CHECK_EXCP	0x00000080	/* Machine check exception */
#define CPUF_CMPXCHG8B		0x00000100	/* CMPXCHG8B instruction */
#define CPUF_LOCAL_APIC		0x00000200	/* CPU contains a local APIC */
#define CPUF_FAST_SYSCALL	0x00000800	/* Fast system call inst */
#define CPUF_MEM_RANGE_REGS	0x00001000	/* memory type range regs */
#define CPUF_PAGE_GLOBAL_EXT	0x00002000	/* page global extensions */
#define CPUF_MACHINE_CHECK_ARCH	0x00004000	/* Machine check architecture */
#define CPUF_CMOVCC		0x00008000	/* CMOVcc instructions */
#define CPUF_PAGE_ATTR_TABLE	0x00010000	/* Page attribute table */
#define CPUF_PSE_36		0x00020000	/* 36-bit page size ext */
#define CPUF_PSN		0x00040000	/* Processor serial number */
#define CPUF_MMX		0x00800000	/* MMX instructions */
#define CPUF_FXSR		0x01000000	/* Fast FP save/restore */
#define CPUF_SSE		0x02000000	/* SEE instructions */
#define CPUF_IA64		0x40000000	/* IA-64 architecture */

/*
 * Cache configuration descriptor entry values.
 */
#define CPUC_NULL			0x00
#define CPUC_CODE_TLB_4K_4W_64E		0x01
#define CPUC_CODE_TLB_4M_FULL_2E	0x02
#define CPUC_DATA_TLB_4K_4W_64E		0x03
#define CPUC_DATA_TLB_4M_4W_8E		0x04
#define CPUC_CODE_L1_32B_4W_8KB		0x06
#define CPUC_CODE_L1_32B_4W_16KB	0x08
#define CPUC_DATA_L1_32B_2W_8KB		0x0a
#define CPUC_DATA_L1_32B_2W_16KB	0x0c
#define CPUC_NO_L2			0x40
#define CPUC_COMB_L2_32B_4W_128KB	0x41
#define CPUC_COMB_L2_32B_4W_256KB	0x42
#define CPUC_COMB_L2_32B_4W_512KB	0x43
#define CPUC_COMB_L2_32B_4W_1M		0x44
#define CPUC_COMB_L2_32B_4W_2M		0x45
#define CPUC_COMB_L2_32B_8W_256KB	0x82


OSKIT_BEGIN_DECLS
/*
 * Generic routine to detect the current processor
 * and fill in the supplied cpu_info structure with all information available.
 * If the vendor ID cannot be determined, it is left a zero-length string.
 * This routine assumes the processor is at least a 386 -
 * since it's a 32-bit function, it wouldn't run on anything less anyway.
 */
void cpuid(struct cpu_info *out_id);

/*
 * Format the information in a cpu_info structure
 * into a human-readable ASCII representation,
 * using the specified printf-like formatting function for output.
 */
void cpu_info_format(
	struct cpu_info *info,
	void (*formatter)(void *data, const char *fmt, ...),
	void *data);

/*
 * Pretty-print a cpu_info structure to the console using printf().
 */
void cpu_info_dump(struct cpu_info *info);

/*
 * Given two cpu_info structures, reduce 'id' to be the minimum
 * (least-common-denominator) of the two structurens.
 * Typically used on SMP systems to determine the basic feature set
 * common across all processors in the system regardless of type.
 */
void cpu_info_min(struct cpu_info *id, const struct cpu_info *id2);
OSKIT_END_DECLS

#endif /* _OSKIT_X86_CPUID_H_ */
