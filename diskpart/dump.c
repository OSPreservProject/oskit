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
 * This routine prints the the partitioning information in a logical form.
 */

#include <sys/types.h> 
#include <stdio.h>
#include <ctype.h>
#include <oskit/diskpart/diskpart.h>

void
diskpart_dump(struct diskpart *array, int level, char part)
{
	int i;
	struct diskpart *subs;

	for (i = 0; i < level; i++)
		printf("  ");

	printf("%c: %d, %d, %d, %d (%d subparts)\n", part,
		array->start, array->size, array->fsys, 
		array->type, array->nsubs);

	subs = array[0].subs;

        for (i = 0; i < array[0].nsubs; i++) {
		diskpart_dump(&subs[i], level+1, 
			      (isalpha(part) ? '1' : 'a') + i);


	}
}

