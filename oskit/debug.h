/*
 * Copyright (c) 1993-1995, 1998 University of Utah and the Flux Group.
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
 *	This file contains definitions for kernel debugging,
 *	which are compiled in on the DEBUG symbol.
 */
#ifndef _OSKIT_DEBUG_H_
#define _OSKIT_DEBUG_H_

#include <oskit/machine/debug.h>

/* Get the assert() macro from the minimal C library's assert.h.
   Sanity check: make sure NDEBUG isn't also defined... */
#if defined(DEBUG) && defined(NDEBUG)
#error Inconsistency: DEBUG and NDEBUG both defined!
#endif
#include <oskit/c/assert.h>

#ifdef DEBUG


/*** Permanent code annotation macros ***/

#define otsan() panic("%s:%d: off the straight and narrow!", __FILE__, __LINE__)

#define do_debug(stmt) stmt

/* XXX not sure how useful this is; just delete? */
#define struct_id_decl		unsigned struct_id;
#define struct_id_init(p,id)	((p)->struct_id = (id))
#define struct_id_denit(p)	((p)->struct_id = 0)
#define struct_id_verify(p,id) \
	({ if ((p)->struct_id != (id)) \
		panic("%s:%d: "#p" (%08x) struct_id should be "#id" (%08x), is %08x\n", \
			__FILE__, __LINE__, (p), (id), (p->struct_id)); \
	})


/*** Macros to provide temporary debugging scaffolding ***/

#include <oskit/c/stdio.h>
#define here() printf("@ %s:%d\n", __FILE__, __LINE__)

#define debugmsg(args) \
	( printf("%s:%d: ", __FILE__, __LINE__), printf args, printf("\n") )

#else /* !DEBUG */

#define otsan()
#define do_debug(stmt)

#define struct_id_decl
#define struct_id_init(p,id)
#define struct_id_denit(p)
#define struct_id_verify(p,id)

#endif /* !DEBUG */

#include <oskit/compiler.h>

OSKIT_BEGIN_DECLS
extern void panic(const char *format, ...) OSKIT_NORETURN;
extern void dump_stack_trace(void); /* defined in libc/<machine>/stack_trace.c */
OSKIT_END_DECLS

#endif /* _OSKIT_DEBUG_H_ */
