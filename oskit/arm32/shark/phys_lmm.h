/*
 * Copyright (c) 1995, 1998, 1999 University of Utah and the Flux Group.
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
 * PC-specific flag bits and priority values
 * for the List Memory Manager (LMM)
 * relevant for kernels managing physical memory.
 */
#ifndef _OSKIT_ARM32_SHARK_PHYS_LMM_H_
#define _OSKIT_ARM32_SHARK_PHYS_LMM_H_

#include <oskit/compiler.h>
#include <oskit/arm32/types.h>

/*
 * The base memory pool is contained in the malloc_lmm.
 * Wrongly named, malloc operates in terms of the memory object.
 */
extern struct lmm malloc_lmm;

/*
 * Assign priorities to each region accordingly
 * so that high memory will be used first when possible,
 * then 16MB memory.
 */
#define LMM_PRI_16MB	-1
#define LMM_PRI_HIGH	0

/*
 * For memory from 0MB to 16MB, only LMMF_16MB will be set.
 * For all memory higher than that, neither will be set.
 *
 * XXX There is no 1MB memory, but LMMF_1MB has leaked out.
 *     The 16MB region is really 4MB on the SHARK, but again the
 *     definition has leaked out. Needs more thought.
 */
#define LMMF_1MB	0x01
#define LMMF_16MB	0x02

OSKIT_BEGIN_DECLS
/*
 * This routine sets up the malloc_lmm with three regions,
 * one for each of the memory types above.
 * You can then call phys_lmm_add() to add memory to those regions.
 */
void phys_lmm_init(void);

/*
 * Call one of these routines to add a chunk of physical memory found
 * to the appropriate region(s) on the malloc_lmm.
 * The provided memory block may be arbitrarily aligned
 * and may cross region boundaries (e.g. the 16MB boundary);
 * it will be shrunken and split apart as necessary.
 * Note that these routines take _physical_ addresses,
 * not virtual addresses as the underlying LMM routines do.
 */
void phys_lmm_add(oskit_addr_t min_pa, oskit_size_t size);

OSKIT_END_DECLS

#endif /* _OSKIT_ARM32_SHARK_PHYS_LMM_H_ */
