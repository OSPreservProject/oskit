/*
 * Copyright (c) 1995, 1996, 1998 University of Utah and the Flux Group.
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
 * Mach Operating System
 * Copyright (c) 1991,1990 Carnegie Mellon University
 * All Rights Reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
/*
 * Processor registers for i386 and i486.
 */
#ifndef	_OSKIT_X86_PROC_REG_H_
#define	_OSKIT_X86_PROC_REG_H_

#include <oskit/compiler.h>
#include <oskit/config.h>

/*
 * CR0
 */
#define	CR0_PG	0x80000000		/*	 enable paging */
#define	CR0_CD	0x40000000		/* i486: cache disable */
#define	CR0_NW	0x20000000		/* i486: no write-through */
#define	CR0_AM	0x00040000		/* i486: alignment check mask */
#define	CR0_WP	0x00010000		/* i486: write-protect kernel access */
#define	CR0_NE	0x00000020		/* i486: handle numeric exceptions */
#define	CR0_ET	0x00000010		/*	 extension type is 80387 */
					/*	 (not official) */
#define	CR0_TS	0x00000008		/*	 task switch */
#define	CR0_EM	0x00000004		/*	 emulate coprocessor */
#define	CR0_MP	0x00000002		/*	 monitor coprocessor */
#define	CR0_PE	0x00000001		/*	 enable protected mode */

/*
 * CR4
 */
#define CR4_VME	0x00000001		/* enable virtual intrs in v86 mode */
#define CR4_PVI	0x00000002		/* enable virtual intrs in pmode */
#define CR4_TSD	0x00000004		/* disable RDTSC in user mode */
#define CR4_DE	0x00000008		/* debug extensions (I/O breakpoints) */
#define CR4_PSE	0x00000010		/* page size extensions */
#define CR4_PAE	0x00000020		/* physical address extensions */
#define CR4_MCE	0x00000040		/* machine check exception */
#define CR4_PGE	0x00000080		/* page global extensions */
#define CR4_PCE	0x00000100		/* enable read perf counter instr */

#ifndef	ASSEMBLER
#ifdef	__GNUC__

OSKIT_INLINE unsigned get_eflags(void);
OSKIT_INLINE void set_eflags(unsigned eflags);
OSKIT_INLINE void cli(void);
OSKIT_INLINE void sti(void);
OSKIT_INLINE void cld(void);
#ifndef __cplusplus
OSKIT_INLINE void std(void);
#endif
OSKIT_INLINE void clts(void);
OSKIT_INLINE unsigned short get_cs(void);
OSKIT_INLINE unsigned short get_ds(void);
OSKIT_INLINE void set_ds(unsigned short ds);
OSKIT_INLINE unsigned short get_es(void);
OSKIT_INLINE void set_es(unsigned short es);
OSKIT_INLINE unsigned short get_fs(void);
OSKIT_INLINE void set_fs(unsigned short fs);
OSKIT_INLINE unsigned short get_gs(void);
OSKIT_INLINE void set_gs(unsigned short gs);
OSKIT_INLINE unsigned short get_ss(void);
OSKIT_INLINE void set_ss(unsigned short ss);

/* Some processors, notably my Am386DX/40,
   seem to have some rather subtle pipeline- or timing-related bugs
   which case really weird things to happen with pushf's and popf's
   that come too close together...  */
#if 1
/* Look for the ``Intel inside'' label...
   For whatever bizzare reason, using the jmp-next-instruction hack
   sometimes causes a trap 15 to occur on the popfl instruction on a
   Pentium Pro 200.  Maybe the pushf's and popf's are too far apart...  */
OSKIT_INLINE unsigned get_eflags(void)
{
	unsigned eflags;
	asm volatile("
		pushfl
		popl %0" : "=r" (eflags));
	return eflags;
}

OSKIT_INLINE void set_eflags(unsigned eflags)
{
	asm volatile("
		pushl %0
		popfl" : : "r" (eflags));
}
#else
OSKIT_INLINE unsigned get_eflags(void)
{
	unsigned eflags;
	asm volatile("
		jmp	1f
	1:	jmp	1f
	1:	jmp	1f
	1:	pushfl
		jmp	1f
	1:	jmp	1f
	1:	jmp	1f
	1:	popl %0" : "=r" (eflags));
	return eflags;
}

OSKIT_INLINE void set_eflags(unsigned eflags)
{
	asm volatile("
		pushl %0
		jmp	1f
	1:	jmp	1f
	1:	jmp	1f
	1:	popfl
		jmp	1f
	1:	jmp	1f
	1:	jmp	1f
	1:	" : : "r" (eflags));
}
#endif

OSKIT_INLINE void cli(void) { asm("cli"); }
OSKIT_INLINE void sti(void) { asm("sti"); }
OSKIT_INLINE void cld(void) { asm("cld"); }
#ifdef __cplusplus
/* in C++, std is a reserved keyword -- so we use a function macro instead */
#define std() asm("std")
#else
OSKIT_INLINE void std(void) { asm("std"); }
#endif
OSKIT_INLINE void clts(void) { asm("clts"); }

OSKIT_INLINE unsigned short get_cs(void)
{
	unsigned short cs;
	asm volatile("movw %%cs,%w0" : "=r" (cs));
	return cs;
}

OSKIT_INLINE unsigned short get_ds(void)
{
	unsigned short ds;
	asm volatile("movw %%ds,%w0" : "=r" (ds));
	return ds;
}
OSKIT_INLINE void set_ds(unsigned short ds)
{
	asm volatile("movw %w0,%%ds" : : "r" (ds));
}

OSKIT_INLINE unsigned short get_es(void)
{
	unsigned short es;
	asm volatile("movw %%es,%w0" : "=r" (es));
	return es;
}
OSKIT_INLINE void set_es(unsigned short es)
{
	asm volatile("movw %w0,%%es" : : "r" (es));
}

OSKIT_INLINE unsigned short get_fs(void)
{
	unsigned short fs;
	asm volatile("movw %%fs,%w0" : "=r" (fs));
	return fs;
}
OSKIT_INLINE void set_fs(unsigned short fs)
{
	asm volatile("movw %w0,%%fs" : : "r" (fs));
}

OSKIT_INLINE unsigned short get_gs(void)
{
	unsigned short gs;
	asm volatile("movw %%gs,%w0" : "=r" (gs));
	return gs;
}
OSKIT_INLINE void set_gs(unsigned short gs)
{
	asm volatile("movw %w0,%%gs" : : "r" (gs));
}

OSKIT_INLINE unsigned short get_ss(void)
{
	unsigned short ss;
	asm volatile("movw %%ss,%w0" : "=r" (ss));
	return ss;
}
OSKIT_INLINE void set_ss(unsigned short ss)
{
	asm volatile("movw %w0,%%ss" : : "r" (ss));
}

#define get_eax() \
    ({ \
	register unsigned int _temp__; \
	asm("movl %%eax, %0" : "=r" (_temp__)); \
	_temp__; \
    })

#define get_ebx() \
    ({ \
	register unsigned int _temp__; \
	asm("movl %%ebx, %0" : "=r" (_temp__)); \
	_temp__; \
    })

#define get_ecx() \
    ({ \
	register unsigned int _temp__; \
	asm("movl %%ecx, %0" : "=r" (_temp__)); \
	_temp__; \
    })

#define get_edx() \
    ({ \
	register unsigned int _temp__; \
	asm("movl %%edx, %0" : "=r" (_temp__)); \
	_temp__; \
    })

#define get_esi() \
    ({ \
	register unsigned int _temp__; \
	asm("movl %%esi, %0" : "=r" (_temp__)); \
	_temp__; \
    })

#define get_edi() \
    ({ \
	register unsigned int _temp__; \
	asm("movl %%edi, %0" : "=r" (_temp__)); \
	_temp__; \
    })

#define get_ebp() \
    ({ \
	register unsigned int _temp__; \
	asm("movl %%ebp, %0" : "=r" (_temp__)); \
	_temp__; \
    })

#define get_esp() \
    ({ \
	register unsigned int _temp__; \
	asm("movl %%esp, %0" : "=r" (_temp__)); \
	_temp__; \
    })

#define get_eflags() \
    ({ \
	register unsigned int _temp__; \
	asm volatile("pushfl; popl %0" : "=r" (_temp__)); \
	_temp__; \
    })

#define get_eip() \
    ({ \
	register unsigned int _temp__; \
	asm("1: movl $1b, %0" : "=r" (_temp__)); \
	_temp__; \
    })

#define	get_cr0() \
    ({ \
	register unsigned int _temp__; \
	asm volatile("mov %%cr0, %0" : "=r" (_temp__)); \
	_temp__; \
    })

#define	set_cr0(value) \
    ({ \
	register unsigned int _temp__ = (value); \
	asm volatile("mov %0, %%cr0" : : "r" (_temp__)); \
     })

#ifdef HAVE_CR4
#define get_cr4() \
    ({ \
	register unsigned int _temp__; \
	asm volatile("mov %%cr4, %0" : "=r" (_temp__)); \
	_temp__; \
    })
#define set_cr4(value) \
    ({ \
	register unsigned int _temp__ = (value); \
	asm volatile("mov %0, %%cr4" : : "r" (_temp__)); \
     })
#else /* not HAVE_CR4 */
#define get_cr4() \
    ({ \
	register unsigned int _temp__; \
	asm volatile(".byte 0x0f,0x20,0xe0" : "=a" (_temp__)); \
	_temp__; \
    })
#define set_cr4(value) \
	asm volatile(".byte 0x0f,0x22,0xe0" : : "a" (value));
#endif /* HAVE_CR4 */

#define get_msw() \
    ({ \
	unsigned short _msw__; \
	asm volatile("smsw %0" : "=r" (_msw__)); \
	_msw__; \
    })

#define	get_cr2() \
    ({ \
	register unsigned int _temp__; \
	asm("mov %%cr2, %0" : "=r" (_temp__)); \
	_temp__; \
    })

#define	get_cr3() \
    ({ \
	register unsigned int _temp__; \
	asm("mov %%cr3, %0" : "=r" (_temp__)); \
	_temp__; \
    })

#define	set_cr3(value) \
    ({ \
	register unsigned int _temp__ = (value); \
	asm volatile("mov %0, %%cr3" : : "r" (_temp__)); \
     })

#define	set_ts() \
	set_cr0(get_cr0() | CR0_TS)

#define	clear_ts() \
	asm volatile("clts")

#define	get_tr() \
    ({ \
	unsigned short _seg__; \
	asm volatile("str %0" : "=rm" (_seg__) ); \
	_seg__; \
    })

#define	set_tr(seg) \
	asm volatile("ltr %0" : : "rm" ((unsigned short)(seg)) )

#define set_gdt(pseudo_desc) \
    ({ \
	asm volatile("lgdt %0" : : "m" ((pseudo_desc)->limit)); \
    })

#define set_idt(pseudo_desc) \
    ({ \
	asm volatile("lidt %0" : : "m" ((pseudo_desc)->limit)); \
    })

#define	get_ldt() \
    ({ \
	unsigned short _seg__; \
	asm volatile("sldt %0" : "=rm" (_seg__) ); \
	_seg__; \
    })

#define	set_ldt(seg) \
	asm volatile("lldt %0" : : "rm" ((unsigned short)(seg)) )

/*
 * Read the 64-bit timestamp counter (TSC) register.
 * Works only on Pentium and higher processors,
 * and in user mode only if the TSD bit in CR4 is not set.
 */
#if HAVE_RDTSC
#define get_tsc() \
    ({ \
	unsigned long low, high; \
	asm volatile("rdtsc" : "=d" (high), "=a" (low)); \
	((unsigned long long)high << 32) | low; \
    })
#define rdmsr(msr) \
    ({ \
	unsigned long low, high; \
	asm volatile("rdmsr" : "=d" (high), "=a" (low) : "c" (msr)); \
	((unsigned long long)high << 32) | low; \
    })
#define wrmsr(msr, high, low) \
    ({ \
	asm volatile("wrmsr" : : "d" (high), "a" (low), "c" (msr)); \
    })
#define wrmsrll(msr, val) \
    ({ \
	asm volatile("wrmsr" : : "A" (val), "c" (msr)); \
    })
#else
#define get_tsc() \
    ({ \
	unsigned long low, high; \
	asm volatile( \
	".byte 0x0f; .byte 0x31" \
	: "=d" (high), "=a" (low)); \
	((unsigned long long)high << 32) | low; \
    })
#define rdmsr(msr) \
    ({ \
	unsigned long low, high; \
	asm volatile(".byte 0x0f; .byte 0x32;" \
 	: "=d" (high), "=a" (low) : "c" (msr)); \
	((unsigned long long)high << 32) | low; \
    })
#define wrmsr(msr, high, low) \
    ({ \
	asm volatile(".byte 0x0f; .byte 0x30" \
	: : "d" (high), "a" (low), "c" (msr)); \
    })
#define wrmsrll(msr, val) \
    ({ \
	asm volatile(".byte 0x0f; .byte 0x30" \
	: : "A" (val), "c" (msr)); \
    })
#endif

/*
 * This doesn't set a processor register,
 * but it's often used immediately after setting one,
 * to flush the instruction queue.
 */
#define flush_instr_queue() \
	asm volatile("
		jmp	0f
		0:
	")

/* Inline functions work fine for 16-bit code as well.  */
#ifdef HAVE_CODE16
#define i16_get_eflags		get_eflags
#define i16_set_eflags		set_eflags
#define i16_cli			cli
#define i16_sti			sti
#define i16_cld			cld
#define i16_std			std
#define i16_clts		clts
#define i16_get_cs		get_cs
#define i16_set_cs		set_cs
#define i16_get_ds		get_ds
#define i16_set_ds		set_ds
#define i16_get_es		get_es
#define i16_set_es		set_es
#define i16_get_fs		get_fs
#define i16_set_fs		set_fs
#define i16_get_gs		get_gs
#define i16_set_gs		set_gs
#define i16_get_ss		get_ss
#define i16_set_ss		set_ss
#define i16_get_cr0		get_cr0
#define i16_set_cr0		set_cr0
#define i16_get_cr3		get_cr3
#define i16_set_cr3		set_cr3
#define i16_get_msw		get_msw
#define i16_set_gdt		set_gdt
#define i16_set_idt		set_idt
#define i16_set_ldt		set_ldt
#define i16_set_tr		set_tr
#define i16_flush_instr_queue	flush_instr_queue
#endif

#endif	/* __GNUC__ */
#endif	/* ASSEMBLER */

#endif	/* _OSKIT_X86_PROC_REG_H_ */
