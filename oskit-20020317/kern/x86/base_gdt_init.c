/*
 * Copyright (c) 1994-1998 University of Utah and the Flux Group.
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
 * Copyright (c) 1991 IBM Corporation 
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation,
 * and that the name IBM not be used in advertising or publicity 
 * pertaining to distribution of the software without specific, written
 * prior permission.
 * 
 * CARNEGIE MELLON AND IBM ALLOW FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON AND IBM DISCLAIM ANY LIABILITY OF ANY KIND FOR
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

#include <oskit/x86/base_vm.h>
#include <oskit/x86/base_gdt.h>
#include <oskit/x86/base_tss.h>

void base_gdt_init(void)
{
	/* Initialize the base TSS descriptor.  */
	fill_descriptor(&base_gdt[BASE_TSS / 8],
			kvtolin(&base_tss), sizeof(base_tss) - 1,
			ACC_PL_K | ACC_TSS | ACC_P, 0);

	/* Initialize the 32-bit kernel code and data segment descriptors
	   to point to the base of the kernel linear space region.  */
	fill_descriptor(&base_gdt[KERNEL_CS / 8],
			kvtolin(0), 0xffffffff,
			ACC_PL_K | ACC_CODE_R, SZ_32);
	fill_descriptor(&base_gdt[KERNEL_DS / 8],
			kvtolin(0), 0xffffffff,
			ACC_PL_K | ACC_DATA_W, SZ_32);

	/* Corresponding 16-bit code and data segment descriptors,
	   typically used when entering and leaving real mode.  */
	fill_descriptor(&base_gdt[KERNEL_16_CS / 8],
			kvtolin(0), 0xffff,
			ACC_PL_K | ACC_CODE_R, SZ_16);
	fill_descriptor(&base_gdt[KERNEL_16_DS / 8],
			kvtolin(0), 0xffff,
			ACC_PL_K | ACC_DATA_W, SZ_16);

	/* Descriptors that direct-map all linear space.  */
	fill_descriptor(&base_gdt[LINEAR_CS / 8],
			0x00000000, 0xffffffff,
			ACC_PL_K | ACC_CODE_R, SZ_32);
	fill_descriptor(&base_gdt[LINEAR_DS / 8],
			0x00000000, 0xffffffff,
			ACC_PL_K | ACC_DATA_W, SZ_32);
}

