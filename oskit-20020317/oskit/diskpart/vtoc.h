/*
 * Copyright (c) 1994, 1996-1998 University of Utah and the Flux Group.
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
 * This file contains the definitions for the VTOC label.
 *
 * It has been modified by the University of Utah for the OSKit,
 * from the Mach version.
 */

#ifndef _OSKIT_DISKLABEL_VTOC_H_
#define _OSKIT_DISKLABEL_VTOC_H_

#define PDLOCATION      29      /* VTOC sector */

#define VTOC_SANE       0x600DDEEE      /* Indicates a sane VTOC */

#define HDPDLOC         29              /* location of pdinfo/vtoc */


/* Here are the VTOC definitions */
#define V_NUMPAR        16              /* maximum number of partitions */

struct localpartition   {
        oskit_u32_t	p_flag;		/*permision flags*/
        oskit_s32_t	p_start;	/*physical start sector no of partition*/
        oskit_s32_t	p_size;		/*# of physical sectors in partition*/
};
typedef struct localpartition localpartition_t;


struct evtoc {
        oskit_u32_t	fill0[6];
        oskit_u32_t	cyls;           /*number of cylinders per drive*/
        oskit_u32_t	tracks;         /*number tracks per cylinder*/
        oskit_u32_t	sectors;        /*number sectors per track*/
        oskit_u32_t	fill1[13];
        oskit_u32_t	version;        /*layout version*/
        oskit_u32_t	alt_ptr;        /*byte offset of alternates table*/
        oskit_u16_t	alt_len;        /*byte length of alternates table*/
        oskit_u32_t	sanity;         /*to verify vtoc sanity*/
        oskit_u32_t	xcyls;          /*number of cylinders per drive*/
        oskit_u32_t	xtracks;        /*number tracks per cylinder*/
        oskit_u32_t	xsectors;       /*number sectors per track*/
        oskit_u16_t	nparts;         /*number of partitions*/
        oskit_u16_t	fill2;          /*pad for 286 compiler*/
        char    label[40];
        struct localpartition part[V_NUMPAR];/*partition headers*/
        char    fill[512-352];
};

#endif /* _OSKIT_DISKLABEL_VTOC_H_ */
