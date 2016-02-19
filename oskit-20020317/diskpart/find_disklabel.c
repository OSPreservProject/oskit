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
 * This is the layer of the partitioning code that searches for a
 * specific partitioning scheme (potentially nested inside annother
 * partitioning scheme).
 *
 * This file currently contains the routines for
 * a BSD-style disklabel.
 *
 */

#include <sys/types.h> 
#include <stdio.h>

#include <oskit/diskpart/diskpart.h>
#include <oskit/diskpart/disklabel.h>


/*
 * This is the routine to locate a BSD disklabel.
 * It should work on the bare drive, or in a dos partition.
 */

int
diskpart_get_disklabel(struct diskpart *array, char *buff, int start,
		void *driver_info, int (*bottom_read_fun)(),
		int max_part) 
{
	struct disklabel *dlp;
	int mybase = start + (512 * LBLLOC)/SECTOR_SIZE, i;
	int count;

        diskpart_check_read((*bottom_read_fun)(driver_info, mybase, buff)); 
                
        dlp = (struct disklabel *)buff;
        if (dlp->d_magic != DISKMAGIC || dlp->d_magic2 != DISKMAGIC) {
		return(0);  /* no partitions added */
        }

	/* note: BSD disklabel offsets are from start of DRIVE -- uuggh */

	count = min(8,max_part);  /* always 8 in a disklabel */

        /* COPY into the array */
	for (i = 0; i < count; i++)
		diskpart_fill_entry(&array[i], /* mybase + */ 
			dlp->d_partitions[i].p_offset,
			dlp->d_partitions[i].p_size,
			NULL, 0, DISKPART_NONE, dlp->d_partitions[i].p_fstype);

		/* note: p_fstype is not the same set as the DOS types */
                        
	return(count);  /* 'always' 8 partitions in disklabel -- if space */

}


