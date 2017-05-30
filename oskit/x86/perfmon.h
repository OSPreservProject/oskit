/*
 * Copyright (c) 2000 University of Utah and the Flux Group.
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
 * Copyright 1996 Massachusetts Institute of Technology
 *
 * Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose and without fee is hereby
 * granted, provided that both the above copyright notice and this
 * permission notice appear in all copies, that both the above
 * copyright notice and this permission notice appear in all
 * supporting documentation, and that the name of M.I.T. not be used
 * in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.  M.I.T. makes
 * no representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied
 * warranty.
 * 
 * THIS SOFTWARE IS PROVIDED BY M.I.T. ``AS IS''.  M.I.T. DISCLAIMS
 * ALL EXPRESS OR IMPLIED WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT
 * SHALL M.I.T. BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Interface to performance-monitoring counters for Intel Pentium and
 * Pentium Pro CPUs.
 */

#ifndef _OSKIT_X86_PERFMON_H_
#define _OSKIT_X86_PERFMON_H_

#include <oskit/compiler.h>
#include <oskit/x86/proc_reg.h>

/* Only two registers */
#define	NPMC	2

/* event selection registers (PPRO) */
#define PMCF_SEL0	0x186
#define PMCF_SEL1	0x187

/* event counter registers (PPRO) */
#define PMCF_CNT0	0xC1
#define PMCF_CNT1	0xC2

/* Flags */
#define	PMCF_USR	0x010000	/* count events in user mode */
#define	PMCF_OS		0x020000	/* count events in kernel mode */
#define	PMCF_E		0x040000	/* use edge-detection mode */
#define	PMCF_PC		0x080000	/* PMx output pin control */
#define	PMCF_INT	0x100000	/* APIC interrupt enable (do not use) */
#define	PMCF_EN		0x400000	/* enable counters */
#define	PMCF_INV	0x800000	/* invert counter mask comparison */

/*
 * Pentium Pro performance counters, from Appendix B.
 */
/* Data Cache Unit */
#define	PMC6_DATA_MEM_REFS	0x43
#define	PMC6_DCU_LINES_IN	0x45
#define	PMC6_DCU_M_LINES_IN	0x46
#define	PMC6_DCU_M_LINES_OUT	0x47
#define	PMC6_DCU_MISS_OUTSTANDING 0x48

/* Instruction Fetch Unit */ 
#define	PMC6_IFU_IFETCH		0x80
#define	PMC6_IFU_IFETCH_MISS	0x81
#define	PMC6_ITLB_MISS		0x85
#define	PMC6_IFU_MEM_STALL	0x86
#define	PMC6_ILD_STALL		0x87

/* L2 Cache */
#define	PMC6_L2_IFETCH		0x28 /* MESI */
#define	PMC6_L2_LD		0x29 /* MESI */
#define	PMC6_L2_ST		0x2a /* MESI */
#define	PMC6_L2_LINES_IN	0x24
#define	PMC6_L2_LINES_OUT	0x26
#define	PMC6_L2_M_LINES_INM	0x25
#define	PMC6_L2_M_LINES_OUTM	0x27
#define	PMC6_L2_RQSTS		0x2e /* MESI */
#define	PMC6_L2_ADS		0x21
#define	PMC6_L2_DBUS_BUSY	0x22
#define	PMC6_L2_DBUS_BUSY_RD	0x23

/* External Bus Logic */
#define	PMC6_BUS_DRDY_CLOCKS	0x62
#define	PMC6_BUS_LOCK_CLOCKS	0x63
#define	PMC6_BUS_REQ_OUTSTANDING 0x60
#define	PMC6_BUS_TRAN_BRD	0x65
#define	PMC6_BUS_TRAN_RFO	0x66
#define	PMC6_BUS_TRAN_WB	0x67
#define	PMC6_BUS_TRAN_IFETCH	0x68
#define	PMC6_BUS_TRAN_INVAL	0x69
#define	PMC6_BUS_TRAN_PWR	0x6a
#define	PMC6_BUS_TRAN_P		0x6b
#define	PMC6_BUS_TRAN_IO	0x6c
#define	PMC6_BUS_TRAN_DEF	0x6d
#define	PMC6_BUS_TRAN_BURST	0x6e
#define	PMC6_BUS_TRAN_ANY	0x70
#define	PMC6_BUS_TRAN_MEM	0x6f
#define	PMC6_BUS_DATA_RCV	0x64
#define	PMC6_BUS_BNR_DRV	0x61
#define	PMC6_BUS_HIT_DRV	0x7a
#define	PMC6_BUS_HITM_DRV	0x7b
#define	PMC6_BUS_SNOOP_STALL	0x7e

/* Floating Point Unit */
#define	PMC6_FLOPS		0xc1 /* counter 0 only */
#define	PMC6_FP_COMP_OPS_EXE	0x10 /* counter 0 only */
#define	PMC6_FP_ASSIST		0x11 /* counter 1 only */
#define	PMC6_MUL		0x12 /* counter 1 only */
#define	PMC6_DIV		0x13 /* counter 1 only */
#define	PMC6_CYCLES_DIV_BUSY	0x14 /* counter 0 only */

/* Memory Ordering */
#define	PMC6_LD_BLOCKS		0x03
#define	PMC6_SB_DRAINS		0x04
#define	PMC6_MISALIGN_MEM_REF	0x05

/* Instruction Decoding and Retirement */
#define	PMC6_INST_RETIRED	0xc0
#define	PMC6_UOPS_RETIRED	0xc2
#define	PMC6_INST_DECODER	0xd0 /* (sic) */

/* Interrupts */
#define	PMC6_HW_INT_RX		0xc8
#define	PMC6_CYCLES_INT_MASKED	0xc6
#define	PMC6_CYCLES_INT_PENDING_AND_MASKED 0xc7

/* Branches */
#define	PMC6_BR_INST_RETIRED	0xc4
#define	PMC6_BR_MISS_PRED_RETIRED 0xc5
#define	PMC6_BR_TAKEN_RETIRED	0xc9
#define	PMC6_BR_MISS_PRED_TAKEN_RET 0xca
#define	PMC6_BR_INST_DECODED	0xe0
#define	PMC6_BTB_MISSES		0xe2
#define	PMC6_BR_BOGUS		0xe4
#define	PMC6_BACLEARS		0xe6

/* Stalls */
#define	PMC6_RESOURCE_STALLS	0xa2
#define	PMC6_PARTIAL_RAT_STALLS	0xd2

/* Segment Register Loads */
#define	PMC6_SEGMENT_REG_LOADS	0x06

/* Clocks */
#define	PMC6_CPU_CLK_UNHALTED	0x79

/*
 * Pentium Performance Counters
 * This list comes from the Harvard people, not Intel.
 */
#define	PMC5_DATA_READ		0
#define	PMC5_DATA_WRITE		1
#define	PMC5_DATA_TLB_MISS	2
#define	PMC5_DATA_READ_MISS	3
#define	PMC5_DATA_WRITE_MISS	4
#define	PMC5_WRITE_M_E		5
#define	PMC5_DATA_LINES_WBACK	6
#define	PMC5_DATA_CACHE_SNOOP	7
#define	PMC5_DATA_CACHE_SNOOP_HIT 8
#define	PMC5_MEM_ACCESS_BOTH	9
#define	PMC5_BANK_CONFLICTS	10
#define	PMC5_MISALIGNED_DATA	11
#define	PMC5_INST_READ		12
#define	PMC5_INST_TLB_MISS	13
#define	PMC5_INST_CACHE_MISS	14
#define	PMC5_SEGMENT_REG_LOAD	15
#define	PMC5_BRANCHES		18
#define	PMC5_BTB_HITS		19
#define	PMC5_BRANCH_TAKEN	20
#define	PMC5_PIPELINE_FLUSH	21
#define	PMC5_INST_EXECUTED	22
#define PMC5_INST_EXECUTED_V	23
#define	PMC5_BUS_UTILIZATION	24
#define	PMC5_WRITE_BACKUP_STALL	25
#define	PMC5_DATA_READ_STALL	26
#define	PMC5_WRITE_E_M_STALL	27
#define	PMC5_LOCKED_BUS		28
#define	PMC5_IO_CYCLE		29
#define	PMC5_NONCACHE_MEMORY	30
#define	PMC5_ADDR_GEN_INTERLOCK	31
#define	PMC5_FLOPS		34
#define	PMC5_BP0_MATCH		35
#define	PMC5_BP1_MATCH		36
#define	PMC5_BP2_MATCH		37
#define	PMC5_BP3_MATCH		38
#define	PMC5_HW_INTR		39
#define	PMC5_DATA_RW		40
#define	PMC5_DATA_RW_MISS	41

#define get_pmc(reg) \
    ({ \
	unsigned long low, high; \
	asm volatile("rdpmc" : "=d" (high), "=a" (low) : "c" (reg)); \
	((unsigned long long)high << 32) | low; \
    })

#undef wrmsr
#define wrmsr(msr, foo, val) \
    ({ \
	asm volatile("wrmsr" : : "A" (val), "c" (msr)); \
    })

OSKIT_INLINE void
enable_perfcounter(int reg, int event, int mask, int flags)
{
	unsigned long long	value = 0;
	
	/* XXX don't set APIC interrupt enable */
	flags &= ~PMCF_INT;

	/* build the control word. */
	value = (flags & (PMCF_OS|PMCF_USR)) | PMCF_EN | (mask << 8) | event;

	switch (reg) {
	case 0:
		/* reset the counter */
		wrmsr(PMCF_CNT0, 0, 0);

		/* And go */
		wrmsr(PMCF_SEL0, 0, value);
		break;

	case 1:
		/* reset the counter */
		wrmsr(PMCF_CNT1, 0, 0);
		
		/* And go */
		wrmsr(PMCF_SEL1, 0, value);
		break;
	}
}

OSKIT_INLINE unsigned long long
read_perfcounter(int reg)
{
	if (reg < 0 || reg >= NPMC)
		return 0;
	
	return get_pmc(reg);
}
#endif /* _OSKIT_X86_PERFMON_H_ */
