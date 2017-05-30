/*
 * Copyright (c) 1994-1996, 1998 University of Utah and the Flux Group.
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
#ifndef _OSKIT_ANNO_H_
#define _OSKIT_ANNO_H_

#include <oskit/compiler.h>
#include <oskit/machine/types.h>

/*
 * One global variable of type 'struct anno_table' should be declared
 * for each separate set of annotation entries that may exist.
 */
struct anno_table
{
	struct anno_entry	*start;
	struct anno_entry	*end;
};

/*
 * Declare a variable of this type in the special "anno" section
 * to register an annotation.
 */
struct anno_entry
{
	oskit_addr_t		val1;
	oskit_addr_t		val2;
	oskit_addr_t		val3;
	struct anno_table	*table;
};

/*
 * This routine should be called once at program startup;
 * it sorts all of the annotation entries appropriately
 * and initializes the annotation tables they reside in.
 */
OSKIT_BEGIN_DECLS
void anno_init(void);

/*
 * Find an entry in the specified annotation table whose val1 field exactly
 * matches the specified value.  If none, returns zero.
 */
struct anno_entry *anno_find_exact(struct anno_table *tab, oskit_addr_t val1);

/*
 * Find an entry in the specified annotation table with the largest val1 field
 * less than or equal to the specified value.
 */
struct anno_entry *anno_find_lower(struct anno_table *tab, oskit_addr_t val1);

/*
 * Debugging: dump all annotation tables using printf. 
 */
void anno_dump(void);
OSKIT_END_DECLS

#include <oskit/machine/anno.h>

#endif /* _OSKIT_ANNO_H_ */
