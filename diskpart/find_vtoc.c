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
 * This file currently contains the routines for VTOC
 * type labels.
 *
 * Note that this is UNTESTED, and little attempt has been made to ensure
 * correctness.
 */

#include <sys/types.h> 
#include <stdio.h>

#include <oskit/diskpart/diskpart.h>
#include <oskit/diskpart/vtoc.h>


/* 
 * NOT TESTED!
 * (Tested on a single VTOC lablel on a 80486 with an IDE drive.)
 */

int
diskpart_get_vtoc(struct diskpart *array, char *buff, int start,
		void *driver_info, int (*bottom_read_fun)(),
		int max_part) 
{
	struct evtoc *evp;
	int n,i;

        diskpart_check_read((*bottom_read_fun)(driver_info, 
		start + PDLOCATION, buff));
        evp = (struct evtoc *)buff;
        if (evp->sanity != VTOC_SANE) {
		return(0);
        }
	n = min(evp->nparts,max_part); /* no longer DISKLABEL limitations... */
   
        for (i = 0; i < n; i++)
		diskpart_fill_entry(&array[i], /* mybase + */ 
			evp->part[i].p_start,
			evp->part[i].p_size,
			NULL, 0, DISKPART_NONE, /* XXX FS_BSDFFS */ 0);

	return(n);  /* (evp->nparts) */
}

