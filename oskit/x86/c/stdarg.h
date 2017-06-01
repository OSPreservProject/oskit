/*
 * Copyright (c) 1994 The University of Utah and the Flux Group.
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
 * Copyright (c) 1993 Carnegie Mellon University.
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
#ifndef _OSKIT_X86_C_STDARG_H_
#define _OSKIT_X86_C_STDARG_H_

#include <oskit/machine/types.h>

#define __va_size(type) ((sizeof(type)+3) & ~0x3)

#ifndef _VA_LIST_
#define _VA_LIST_
typedef	oskit_va_list va_list;
#endif

#define	va_start(pvar, lastarg)			\
	((pvar) = (char*)(void*)&(lastarg) + __va_size(lastarg))
#define	va_end(pvar)
#define	va_arg(pvar,type)			\
	((pvar) += __va_size(type),		\
	 *((type *)((pvar) - __va_size(type))))

#endif /* _OSKIT_X86_C_STDARG_H_ */
