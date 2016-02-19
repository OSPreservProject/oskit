/*
 * Copyright (c) 1996-1997 University of Utah and the Flux Group.
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
 * Copyright (c) 1991,1990,1989 Carnegie Mellon University
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

#ifndef _OSKIT_DISKPART_DEC_H_
#define _OSKIT_DISKPART_DEC_H_

/*
 * Definition of the DEC disk label,
 * which is located (you guessed it)
 * at the end of the 4.3 superblock.
 */

struct dec_partition_info {
        unsigned int    n_sectors;      /* how big the partition is */
        unsigned int    offset;         /* sector no. of start of part. */
};

typedef struct {
        int     magic;
#       define  DEC_LABEL_MAGIC         0x032957
        int     in_use;
        struct  dec_partition_info partitions[8];
} dec_label_t;

/*
 * Physical location on disk.
 * This is independent of the filesystem we use,
 * although of course we'll be in trouble if we
 * screwup the 4.3 SBLOCK..
 */

#define DEC_LABEL_BYTE_OFFSET   ((2*8192)-sizeof(dec_label_t))


/*
 * Definitions for the primary boot information
 * This is common, cuz the prom knows it.
 */

typedef struct {
        int             pad[2];
        unsigned int    magic;
#       define          DEC_BOOT0_MAGIC 0x2757a
        int             mode;
        unsigned int    phys_base;
        unsigned int    virt_base;
        unsigned int    n_sectors;
        unsigned int    start_sector;
} dec_boot0_t;

typedef struct {
        dec_boot0_t     vax_boot;
                                        /* BSD label still fits in pad */
        char                    pad[0x1e0-sizeof(dec_boot0_t)];
        unsigned long           block_count;
        unsigned long           starting_lbn;
        unsigned long           flags;
        unsigned long           checksum; /* add cmpl-2 all but here */
} alpha_boot0_t;

#endif /* _OSKIT_DISKPART_DEC_H_ */
