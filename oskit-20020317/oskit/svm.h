/*
 * Copyright (c) 1996, 1998, 1999, 2000 University of Utah and the Flux Group.
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
 * Public header file for SVM
 */
#ifndef _OSKIT_SVM_H_
#define _OSKIT_SVM_H_

#include <oskit/machine/types.h>
#include <oskit/io/absio.h>
#include <oskit/page.h>
#include <assert.h>

/*
 * Protection bits, which will map into real PDE protection bits.
 */
#define	SVM_PROT_READ	0x01	/* pages can be read */
#define	SVM_PROT_WRITE	0x02	/* pages can be written */
#define SVM_PROT_WIRED  0x04	/* page is wired down */
#define SVM_PROT_RESVD  0x80	/* Reserved memory */

/*
 * Flag bits for svm_alloc().
 */
#define SVM_ALLOC_FIXED 0x01	/* fixed placement */
#define SVM_ALLOC_WIRED	0x02	/* region is wired down */

/*
 * Flag bits (possibly) returned by svm_incore().
 */
#define SVM_INCORE_INCORE           0x1 /* Page is in core */
#define SVM_INCORE_REFERENCED       0x2 /* Page has been referenced */
#define SVM_INCORE_MODIFIED         0x4 /* Page has been modified */

/*
 * Status returned by the SEGV handler, or in the absence of a handler,
 * by svm_fault internally.
 */
#define SVM_FAULT_OKAY		0x0	/* Fault was handled */
#define SVM_FAULT_PAGEFLT	0x1	/* A complete page fault */
#define SVM_FAULT_PROTFLT	0x2	/* A protection fault */

/*
 * Prototypes
 */
void		svm_init(oskit_absio_t *pager_absio);
int		svm_pager_init(oskit_absio_t *pager_absio);
void		svm_set_segv_handler(int (*handler)(oskit_addr_t la, int rw));
int		svm_alloc(oskit_addr_t *addrp, oskit_size_t len,
			int prot, int flags);
int		svm_protect(oskit_addr_t addr, oskit_size_t len, int prot);
int		svm_dealloc(oskit_addr_t addr, oskit_size_t len);
int		svm_mapped(oskit_addr_t addr, oskit_size_t len);
int		svm_incore(oskit_addr_t addr, oskit_size_t len, char *pagemap);

#endif /* _OSKIT_SVM_H_ */
