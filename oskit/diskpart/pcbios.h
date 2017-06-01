/*
 * Copyright (c) 1996, 1997 University of Utah and the Flux Group.
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

/*
 * Definition of the i386AT disk label, which lives inside sector 0.
 * This is the info the BIOS knows about, which we use for bootstrapping.
 * It is common across all disks known to BIOS.
 */

#ifndef _OSKIT_DISKLABEL_PCBIOS_H_
#define _OSKIT_DISKLABEL_PCBIOS_H_

struct bios_partition_info {

        unsigned char   bootid; /* bootable or not */
#       define BIOS_BOOTABLE    128

        unsigned char   beghead;/* beginning head, sector, cylinder */
        unsigned char   begsect;/* begcyl is a 10-bit number. High 2 bits */
        unsigned char   begcyl; /*     are in begsect. */

        unsigned char   systid; /* filesystem type */
#       define  UNIXOS          99      /* GNU HURD? */
#       define  BSDOS          165      /* 386BSD */
#       define  LINUXSWAP      130
#       define  LINUXOS        131
#       define  DOS_EXTENDED    05      /* container for logical partitions */

#       define  HPFS            07      /* OS/2 Native */
#       define  OS_2_BOOT       10      /* OS/2 Boot Manager */
#       define  DOS_12          01      /* 12 bit FAT */
#       define  DOS_16_SMALL    04      /* < 32MB */
#       define  DOS_16          06      /* >= 32MB */

/* 
 *These numbers can't be trusted because of newer, larger drives
*/
        unsigned char   endhead;/* ending head, sector, cylinder */
        unsigned char   endsect;/* endcyl is a 10-bit number.  High 2 bits */
        unsigned char   endcyl; /*     are in endsect. */

/*
 * The offset and n_sectors fields should be correct, regardless of the
 * drive size, and should be the numbers used.
 */
        unsigned long   offset;
        unsigned long   n_sectors;
};

typedef struct {
/*      struct bios_partition_info      bogus compiler alignes wrong
                        partitions[4];
*/
        char            partitions[4*sizeof(struct bios_partition_info)];
        unsigned short  magic;
#       define  BIOS_LABEL_MAGIC        0xaa55
} bios_label_t;

/*
 * Physical location on disk.
 */

#define BIOS_LABEL_BYTE_OFFSET  446

/*
 * Definitions for the primary boot information
 * This _is_ block 0
 */

#define BIOS_BOOT0_SIZE BIOS_LABEL_BYTE_OFFSET

typedef struct {
        char            boot0[BIOS_BOOT0_SIZE]; /* boot code */
/*      bios_label_t label;     bogus compiler alignes wrong */
        char            label[sizeof(bios_label_t)];
} bios_boot0_t;


#endif /* _OSKIT_DISKLABEL_PCBIOS_H_ */
