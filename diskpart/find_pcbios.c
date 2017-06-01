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
 * This file contains the partition code that searches for a PC BIOS label.
 * It won't work on big-endian (RISC) machines, as PCs are little-endian.
 */


#include <sys/types.h> 
#include <stdio.h>

#include <oskit/diskpart/diskpart.h>
#include <oskit/diskpart/pcbios.h>

/* Parts of the following code is derived from the SCSI/IDE Mach drivers */

int
diskpart_get_dos(struct diskpart *array, char *buff, int start,
		void *driver_info, int (*bottom_read_fun)(),
		int max_part)
{

	bios_label_t *table;     /* DOS partition table in the boot sector */
	struct bios_partition_info *entry; /* entry in the partition table */

	int count, i, j;
	int ext = -1, mystart = start, mybase;
	int first = 1;

	/*
	 * Note: start is added, although a start != 0 is meaningless
	 * to DOS and anything else...
	 */

	/* check the boot sector for a partition table.  Always in sector 0 */
        diskpart_check_read((*bottom_read_fun)(driver_info, start, buff));

        /* Check for valid partition table. */
        table = (bios_label_t *)&buff[BIOS_LABEL_BYTE_OFFSET];
        if (table->magic != BIOS_LABEL_MAGIC) {
                return(0);   /* didn't add any partitions */
        }

	count = min(4,max_part);	/* always 4 (primary) partitions */

	j = 0;
	for (i = 0, entry = (struct bios_partition_info *)table->partitions;
		i < 4 ; entry++,i++)

		if (entry->n_sectors != 0) j++; /* count non-empty partition */

	if (j == 0) return(0);	/* all are empty -- didn't find it */

	/* fill the next 4 entries in the array */
	for (i = 0, entry = (struct bios_partition_info *)table->partitions;
		i < count; i++, entry++) {

		diskpart_fill_entry(&array[i], entry->offset, entry->n_sectors, 
			NULL, 0, DISKPART_NONE, entry->systid);
		if ((entry->systid == DOS_EXTENDED) && (ext < 0)) {
			mystart += entry->offset; 
			ext = i;
		}
	}

	/*
	 * If there is an extended partition, find all the logical partitions
	 * note: logical start at '5' (extended is one of the numbered 1-4)
	 */

	/*
	 * Logical partitions 'should' be nested inside the primary, but
	 * then it would be impossible to NAME a disklabel inside a logical
	 * partition, which would be nice to do
	 */

	while (ext >= 0) {
		entry = &(((struct bios_partition_info *)
			table->partitions)[ext]);

		/* read the EXTENDED partition table */
		if (first) {
			mybase = mystart;
			first = 0;
		} else {
			mybase = mystart + entry->offset;
		}

		diskpart_check_read((*bottom_read_fun)(driver_info, 
			mybase, buff));

        	if (table->magic != BIOS_LABEL_MAGIC) {
               		return(count);  /*don't add any more partitions*/
        	}

		/* just in case more than one partition is there...*/
		/* search the EXTENDED partition table */
         	for (j = 0, ext = -1,
			entry = (struct bios_partition_info *)
			     table->partitions;
               		j < 4; j++, entry++) {    

			if (entry->systid && (entry->systid!=DOS_EXTENDED)) {
			   if (count < max_part) {
				diskpart_fill_entry(&array[count], 
				    mybase + entry->offset, 
				    entry->n_sectors, NULL, 0, DISKPART_NONE, 
				    entry->systid);
				count++; }
			    else {
				return(count);
			    }
			} else if ((ext < 0) &&
					(entry->systid == DOS_EXTENDED)) {
				ext = j;
				/* recursivly search the chain here */
			}
       		}
	}
	return(count);  /* number dos partitions found */

}


