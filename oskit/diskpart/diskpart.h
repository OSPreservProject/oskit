/*
 * Copyright (c) 1996-1999 University of Utah and the Flux Group.
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

#ifndef	_OSKIT_DISKLABEL_H_
#define	_OSKIT_DISKLABEL_H_

#include <oskit/io/blkio.h>

#include <oskit/compiler.h>

#define SECTOR_SIZE 512

#define LBLLOC          1               /* label block for xxxbsd */


/* These are the partition types that can contain sub-partitions */
/* enum types... */
#define DISKPART_NONE   0       /* smallest piece flag !?! */
#define DISKPART_DOS    1
#define DISKPART_BSD    2
#define DISKPART_VTOC   3
#define DISKPART_OMRON  4
#define DISKPART_DEC    5       /* VAX disks? */
#define DISKPART_UNKNOWN 99



/*
 * This is the basic partition structure.  an array of these is
 * filled, with element 0 being the whole drive, element 1-n being the
 * n top-level partitions, followed by 0+ groups of 1+ sub-partitions.
 */
typedef struct diskpart {
        short type;     /* DISKPART_xxx (see above) */
        short fsys;     /* file system (if known) */
        int nsubs;      /* number of sub-slices */
        struct diskpart *subs;  /* pointer to the sub-partitions */
        int start;      /* relative to the start of the DRIVE */
        int size;       /* # sectors in this piece */
} diskpart_t;


/* MOVE THIS TO A PRIVATE HEADER FILE */
#ifndef min
#define min(x,y) ((x)<(y)?(x):(y))
#endif

OSKIT_BEGIN_DECLS
/*
 * diskpart_check_read is used to provide an immediate return if
 * bottom_read_fun encounters an unrecoverable error.  If an immediate
 * return is not desired, the data returned should be bzero'ed by
 * bottom_read_fun, and 0 returned.  This may be the case, for
 * example, if the sector number is past the end of the disk.
 * bottom_read_fun can return errors through gobal variables as well.
 */
#define diskpart_check_read(w) { if (w) return(w); }

/*
 *
 */
void diskpart_fill_entry(struct diskpart *array, int start, int size,
                struct diskpart *subs, int nsubs, short type, short fsys);

/*
 * This prints an entry pointed to by array, shifted right by level,
 * with the part name.  Any sub-partitions it contains are
 * automatically printed at level+1.  If part is a letter, the
 * sub-parts are numbered, else the subparts are lettered.
 */
void diskpart_dump(struct diskpart *array, int level, char part);

/*
 * This is the main function for getting partition information for a
 * drive.  driver_info is passed to the bottom_read_fun, so the
 * structure is defined by the user.  bottom_read_fun takes three
 * parameters (driver_info, int sector_number, char *buffer).  It
 * should read the sector number and place the data into the buffer.
 * The other parameters to diskpart_get_partition are a pointer to the
 * storage for the partition information, an integer specifying the
 * number of entries that may be used (array size), and an integer
 * specifying the size of the disk, which is used for the whole-disk
 * partition.
 */
int diskpart_get_partition(void *driver_info, 
			int (*bottom_read_fun)(void *driver_info, int sector, 
					       char *buf),
                        struct diskpart *array, int array_size,
                        int disk_size);

/*
 * This is an alternative interface to diskpart_get_partition.
 * It takes a oskit_blkio_t instead of a callback.
 */
int diskpart_blkio_get_partition(oskit_blkio_t *b, struct diskpart *array,
				 int array_size);

/*
 * These functions are normally called by diskpart_get_partition.
 * However, they can be called by the user if it is important to check
 * is a particular type of partitioning scheme is being used.
 */
int diskpart_get_disklabel(struct diskpart *array, char *buff, int start,
                void *driver_info, 
		int (*bottom_read_fun)(void *driver_info, int sector, 
				       char *buf),
                int max_part);

int diskpart_get_dos(struct diskpart *array, char *buff, int start,
                void *driver_info, 
		int (*bottom_read_fun)(void *driver_info, int sector, 
				       char *buf),
                int max_part);

int diskpart_get_vtoc(struct diskpart *array, char *buff, int start,
                void *driver_info, 
		int (*bottom_read_fun)(void *driver_info, int sector, 
				       char *buf),
		int max_part);

/*
 * These functions are not provided.  These are merely placeholders in
 * case support is added.
 */
int diskpart_get_dec(struct diskpart *array, char *buff, int start,
                void *driver_info, 
		int (*bottom_read_fun)(void *driver_info, int sector, 
				       char *buf),
                int max_part);

int diskpart_get_omron(struct diskpart *array, char *buff, int start,
                void *driver_info, 
		int (*bottom_read_fun)(void *driver_info, int sector, 
				       char *buf),
                int max_part);

/*
 * These are the sample lookup routines.
 * They get called with a pointer to the first struct diskpart entry
 * and information indicating which partition should be returned.
 * They return a pointer to the partition specified, NULL if invallid.
 */
struct diskpart *diskpart_lookup_bsd_compat(struct diskpart *array,
					    short slice, short part);

struct diskpart *diskpart_lookup_bsd_string(struct diskpart *array,
					    const char *name);
OSKIT_END_DECLS

/*
 * Here are two more lookup routines.
 * They are analagous to the above but take a oskit_blkio_t and return
 * another one which is a "subset" of the first one and represents
 * the partition.
 */
struct diskpart *diskpart_blkio_lookup_bsd_compat(struct diskpart *array,
						  short slice, short part,
						  oskit_blkio_t *b,
						  oskit_blkio_t **out_b);

struct diskpart *diskpart_blkio_lookup_bsd_string(struct diskpart *array,
						  const char *name,
						  oskit_blkio_t *b,
						  oskit_blkio_t **out_b);

#endif	/* _OSKIT_DISKLABEL_H_ */
