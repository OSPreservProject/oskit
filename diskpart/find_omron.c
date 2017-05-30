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
 * This file currently contains the routines for OMRON
 * type labels.
 *
 * Note that this is UNTESTED, and no attempt has been made to ensure
 * correctness.
 */

#include <sys/types.h> 
#include <stdio.h>

#include <oskit/diskpart/diskpart.h>
#include <oskit/diskpart/omron.h>


/*
 * NOT TESTED!
 */
#if 0 
/*
 * Not only is this code not tested, it is obviously broken. -BAD
 */
int diskpart_get_omron(struct diskpart *array, char *buff, int start,
		void *driver_info, int (*bottom_read_fun)(),
		int max_part) 
{

        struct disklabel        *label;

        /* here look for an Omron label */
        register omron_label_t  *part;
        int                             i;

#	if (PARTITION_DEBUG) 
		printf("Looking for Omron label...\n");
#	endif

        diskpart_check_read((*bottom_read_fun)(driver_info, start+
		OMRON_LABEL_BYTE_OFFSET/SECTOR_SIZE, buff));

        part = (omron_label_t*)&buff[OMRON_LABEL_BYTE_OFFSET%SECTOR_SIZE];
        if (part->magic == OMRON_LABEL_MAGIC) {
#		if (PARTITION_DEBUG) 
        	        printf("{Using OMRON label}");
#		endif
                for (i = 0; i < 8; i++) {
                        label->d_partitions[i].p_size = part->partitions[i].n_sectors;
                        label->d_partitions[i].p_offset = part->partitions[i].offset;
                }
                bcopy(part->packname, label->d_packname, 16);
                label->d_ncylinders = part->ncyl;
                label->d_acylinders = part->acyl;
                label->d_ntracks = part->nhead;
                label->d_nsectors = part->nsect;
                /* Many disks have this wrong, therefore.. */
#if 0
                label->d_secperunit = part->maxblk;
#else
                label->d_secperunit = label->d_ncylinders * label->d_ntracks *
                                        label->d_nsectors;
#endif 0
   
                return(8); 
        }
#	if (PARTITION_DEBUG) 
        	printf("No Omron label found.\n");
#	endif
	return(0);
}
#endif 0

