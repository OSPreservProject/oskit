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
 * This contains the code that searches a disk for partitions.
 * It is possible (and straightforward) to modify this code to find every
 * possible combination of nestings, to any depth.
 * However, this functionality was not currently required.
 *
 * The future plan is to have it recursivly search all partitions found 
 * for any partitions of any type.
 */

#include <sys/types.h> 
#include <stdio.h>

#include <oskit/diskpart/diskpart.h>
#include <oskit/diskpart/pcbios.h> /*XXX*/

/* I check the return type after each call to get partition information */
#define diskpart_check_get(w) { if ((w < 0) || (w > array_size)) return(w); }


/* array is a pointer to an array of partition_info structures */
/* array_size is the number of pre-allocated entries there are */

int
diskpart_get_partition(void *driver_info, int (*bottom_read_fun)(),
		       struct diskpart *array, int array_size,
		       int disk_size)
{
	char buff[SECTOR_SIZE];
	int i,n,cnt;
	int arrsize;

	/* first fill in the entire disk stuff */
	/* or should the calling routine do that already? */

	diskpart_fill_entry(array, 0, disk_size, NULL, 0, -1, -1);

	arrsize = array_size -1;  /* 1 for whole disk */

	/* search for dos partition table */
	/* find all the partitions (including logical) */

	n = diskpart_get_dos(&array[1], buff, 0,
                driver_info, (bottom_read_fun), arrsize);
	diskpart_check_get(n);

	if (n > 0) { /* whole drive is DOS type */
		diskpart_fill_entry(array, 0, disk_size, &array[1], n, 
			DISKPART_DOS, 256+DISKPART_DOS);
		arrsize -= n;


		/* search each one for a BSD disklabel (iff BSDOS) */
		/* note: searching logical partitions as well */
		for (i = 0; i < n; i++)
			if (array[i+1].fsys == BSDOS) {
				cnt = diskpart_get_disklabel(&array[n+1], buff, 
					array[i+1].start,
                			driver_info, (bottom_read_fun),
                			arrsize);
				diskpart_check_get(cnt);

				if (cnt > 0) {
					arrsize -= cnt;
					diskpart_fill_entry(&array[i+1],
						array[i+1].start,
						array[i+1].size, &array[n+1], 
						cnt, DISKPART_BSD,
						array[i+1].fsys);
				}
				n += cnt;
			}

		/* search for VTOC -- in a DOS partition as well */
		for (i = 0; i < n; i++)
			if (array[i+1].fsys == UNIXOS) {
                                cnt = diskpart_get_vtoc(&array[n+1], buff,   
                                        array[i+1].start,
                                        driver_info, (bottom_read_fun),
                                        arrsize);
				diskpart_check_get(cnt);

                                if (cnt > 0) {
					arrsize -= cnt;
                                        diskpart_fill_entry(&array[i+1],
						array[i+1].start,
                                                array[i+1].size, &array[n+1],
                                                cnt, DISKPART_VTOC,
						array[i+1].fsys);
				}
				n += cnt;
			}
	}

	/* search for only disklabel */
	if (n == 0) {
		diskpart_fill_entry(array, 0, disk_size, &array[1], n,
			DISKPART_BSD, 256+DISKPART_BSD);
		n = diskpart_get_disklabel(&array[1], buff, 0, driver_info, 
			(bottom_read_fun), arrsize);
		diskpart_check_get(n);

		if (n == 0) {        /* HP-BSD labels ... */
			n = diskpart_get_disklabel(&array[1], buff, 1, 
				driver_info, (bottom_read_fun), arrsize);
			diskpart_check_get(n);
		}
		if (n > 0) {
			diskpart_fill_entry(array, 0, disk_size, &array[1], n, 
				DISKPART_BSD, 256+DISKPART_BSD);
		}
	}

	/* search for only VTOC -- NOT TESTED! */
	if (n == 0) {
		diskpart_fill_entry(array, 0, disk_size, &array[1], n, 
			DISKPART_VTOC, 256+DISKPART_VTOC);
		n = diskpart_get_vtoc(&array[1], buff, 0, driver_info, 
			(bottom_read_fun), arrsize);
		diskpart_check_get(n);

		if (n > 0) {
			diskpart_fill_entry(array, 0, disk_size, &array[1], n, 
			DISKPART_VTOC, 256+DISKPART_VTOC);
		}
	}

#if 0
	/* search for only omron -- NOT TESTED! */
	if (n == 0) {
		diskpart_fill_entry(array, 0, disk_size, &array[1], n, 
			DISKPART_OMRON, 256+DISKPART_OMRON);
		n = diskpart_get_omron(&array[1], buff, 0,driver_info, 
			(bottom_read_fun), arrsize);
		diskpart_check_get(n);
	}

	/* search for only dec -- NOT TESTED! */
	if (n == 0) {
		diskpart_fill_entry(array, 0, disk_size, &array[1], n, 
			DISKPART_DEC, 256+DISKPART_DEC);
		n = diskpart_get_dec(&array[1], buff, 0, driver_info, 
			(bottom_read_fun), arrsize);
		diskpart_check_get(n);
	}
#endif 0


	return(n);
}

