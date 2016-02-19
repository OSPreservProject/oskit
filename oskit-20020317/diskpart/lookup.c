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
 * This file contains the code to convert a FreeBSD style slice and
 * partition number into an index into the partition table.
 *
 * This allows a disklable to be inside a DOS partition.
 */

#include <sys/types.h> 
#include <stdio.h>

#include <oskit/diskpart/diskpart.h>


/*
 * This will be called by every routine that needs to access partition
 * info based on a device number.
 */

/*
 * It may be better to cache the results of the call to the lookup
 * function.  The address returned cound then be used as the `handle',
 * which would enable much faster accesses to the data.  These could
 * be compared to see if the same partition is opened more than once.
 * However, multiple aliases in the partitioning information will not
 * be detected this easily -- the bounds will still have to be checked
 * by the user.
 */

/*
 * Slice 0 is either the whole disk (when partition is also 0), or is
 * aliased (compatability slice), while Slice 1+ represent actual
 * top-level (typically DOS) partitions.
 */

struct diskpart *
diskpart_lookup_bsd_compat(struct diskpart *array, short slice, short part)
{
        struct diskpart *s;

	/* S0 is either the whole disk (p=0) or mapped to annother partition */
        if (slice == 0) {
		/* s0, p0 => whole disk */
                if (part == 0)
                        return &array[0];

		/* check compatability slice for mapping to BSD partition */
                if (array[0].type == DISKPART_DOS) {
                        int i;
                        for (i = 0; i < array[0].nsubs; i++) {
                                s = &array[0].subs[i];
                                if (s->type == DISKPART_BSD) {
                                        if (part > s->nsubs)
                                                return 0;
                                        return (&s->subs[part-1]);
                                }
                        }

			/* check for mapping to VTOC partition */
                        for (i = 0; i < array[0].nsubs; i++) {
                                s = &array[0].subs[i];
                                if (s->type == DISKPART_VTOC) { /* VTOC/UNIX */ 
                                        if (part > s->nsubs)
                                                return 0;
                                        return (&s->subs[part-1]);
                                }
                        }
                }

                if (part > array[0].nsubs)
                        return 0;
                return(&array[0].subs[part-1]);
        } else {
		/* slice != 0, find the DOS slice */
                if (array[0].type != DISKPART_DOS)
                        return 0;
                if (slice > array[0].nsubs) {
                        return 0;
		}
                s = &array[0].subs[slice-1];

                if (part == 0)
                        return (s);
                if (part > s->nsubs)
                        return 0;
                return (&s->subs[part-1]);
        }
}

