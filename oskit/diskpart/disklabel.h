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
 * Copyright (c) 1987, 1988, 1993
 *	The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)disklabel.h	8.1 (Berkeley) 6/2/93
 */

/*
 * This file was modified from the FreeBSD 2.1 version for use in the 
 * University of Utah's OSKit.
 */

/*
 * Each disk has a label which includes information about the hardware
 * disk geometry, filesystem partitions, and drive specific information.
 * The label is in block 0 or 1, possibly offset from the beginning
 * to leave room for a bootstrap, etc.
 */

/* XXX these should be defined per controller (or drive) elsewhere, not here! */
#if defined(i386) || defined(HPOSF)
#define LABELSECTOR	1			/* sector containing label */
#define LABELOFFSET	0			/* offset of label in sector */
#endif

#ifndef	LABELSECTOR
#define LABELSECTOR	0			/* sector containing label */
#endif

#ifndef	LABELOFFSET
#define LABELOFFSET	64			/* offset of label in sector */
#endif

#define DISKMAGIC	((oskit_u32_t) 0x82564557)	/* The disk magic number */
#ifndef MAXPARTITIONS
#define	MAXPARTITIONS	8
#endif


#ifndef LOCORE
struct disklabel {
	oskit_u32_t	d_magic;	/* the magic number */
	oskit_s16_t	d_type;		/* drive type */
	oskit_s16_t	d_subtype;	/* controller/d_type specific */
	char	d_typename[16];		/* type name, e.g. "eagle" */
	/* 
	 * d_packname contains the pack identifier and is returned when
	 * the disklabel is read off the disk or in-core copy.
	 * d_boot0 and d_boot1 are the (optional) names of the
	 * primary (block 0) and secondary (block 1-15) bootstraps
	 * as found in /usr/mdec.  These are returned when using
	 * getdiskbyname(3) to retrieve the values from /etc/disktab.
	 */
#if defined(KERNEL) || defined(STANDALONE)
	char	d_packname[16];			/* pack identifier */ 
#else
	union {
		char	un_d_packname[16];	/* pack identifier */ 
		struct {
			char *un_d_boot0;	/* primary bootstrap name */
			char *un_d_boot1;	/* secondary bootstrap name */
		} un_b; 
	} d_un; 
#define d_packname	d_un.un_d_packname
#define d_boot0		d_un.un_b.un_d_boot0
#define d_boot1		d_un.un_b.un_d_boot1
#endif	/* ! KERNEL or STANDALONE */
			/* disk geometry: */
	oskit_u32_t	d_secsize;	/* # of bytes per sector */
	oskit_u32_t	d_nsectors;	/* # of data sectors per track */
	oskit_u32_t	d_ntracks;	/* # of tracks per cylinder */
	oskit_u32_t	d_ncylinders;	/* # of data cylinders per unit */
	oskit_u32_t	d_secpercyl;	/* # of data sectors per cylinder */
	oskit_u32_t	d_secperunit;	/* # of data sectors per unit */
	/*
	 * Spares (bad sector replacements) below
	 * are not counted in d_nsectors or d_secpercyl.
	 * Spare sectors are assumed to be physical sectors
	 * which occupy space at the end of each track and/or cylinder.
	 */
	oskit_u16_t	d_sparespertrack; /* # of spare sectors per track */
	oskit_u16_t	d_sparespercyl;	/* # of spare sectors per cylinder */
	/*
	 * Alternate cylinders include maintenance, replacement,
	 * configuration description areas, etc.
	 */
	oskit_u32_t	d_acylinders;	/* # of alt. cylinders per unit */

			/* hardware characteristics: */
	/*
	 * d_interleave, d_trackskew and d_cylskew describe perturbations
	 * in the media format used to compensate for a slow controller.
	 * Interleave is physical sector interleave, set up by the formatter
	 * or controller when formatting.  When interleaving is in use,
	 * logically adjacent sectors are not physically contiguous,
	 * but instead are separated by some number of sectors.
	 * It is specified as the ratio of physical sectors traversed
	 * per logical sector.  Thus an interleave of 1:1 implies contiguous
	 * layout, while 2:1 implies that logical sector 0 is separated
	 * by one sector from logical sector 1.
	 * d_trackskew is the offset of sector 0 on track N
	 * relative to sector 0 on track N-1 on the same cylinder.
	 * Finally, d_cylskew is the offset of sector 0 on cylinder N
	 * relative to sector 0 on cylinder N-1.
	 */
	oskit_u16_t	d_rpm;		/* rotational speed */
	oskit_u16_t	d_interleave;	/* hardware sector interleave */
	oskit_u16_t	d_trackskew;	/* sector 0 skew, per track */
	oskit_u16_t	d_cylskew;	/* sector 0 skew, per cylinder */
	oskit_u32_t	d_headswitch;	/* head switch time, usec */
	oskit_u32_t	d_trkseek;	/* track-to-track seek, usec */
	oskit_u32_t	d_flags;	/* generic flags */
#define NDDATA 5
	oskit_u32_t	d_drivedata[NDDATA]; /* drive-type specific information */
#define NSPARE 5
	oskit_u32_t	d_spare[NSPARE]; /* reserved for future use */
	oskit_u32_t	d_magic2;	/* the magic number (again) */
	oskit_u16_t	d_checksum;	/* xor of data incl. partitions */

			/* filesystem and partition information: */
	oskit_u16_t	d_npartitions;	/* number of partitions in following */
	oskit_u32_t	d_bbsize;	/* size of boot area at sn0, bytes */
	oskit_u32_t	d_sbsize;	/* max size of fs superblock, bytes */
	struct	partition {		/* the partition table */
		oskit_u32_t	p_size;	/* number of sectors in partition */
		oskit_u32_t	p_offset; /* starting sector */
		oskit_u32_t	p_fsize; /* filesystem basic fragment size */
		oskit_u8_t	p_fstype; /* filesystem type, see below */
		oskit_u8_t	p_frag;	 /* filesystem fragments per block */
		union {
			oskit_u16_t	cpg;	/* UFS: FS cylinders per group */
			oskit_u16_t	sgs;	/* LFS: FS segment shift */
		} __partition_u1;
#define	p_cpg	__partition_u1.cpg
#define	p_sgs	__partition_u1.sgs
	} d_partitions[MAXPARTITIONS];	/* actually may be more */
};
#else /* LOCORE */
	/*
	 * offsets for asm boot files.
	 */
	.set	d_secsize,40
	.set	d_nsectors,44
	.set	d_ntracks,48
	.set	d_ncylinders,52
	.set	d_secpercyl,56
	.set	d_secperunit,60
	.set	d_end_,276		/* size of disk label */
#endif /* LOCORE */

/* d_type values: */
#define	DTYPE_SMD		1		/* SMD, XSMD; VAX hp/up */
#define	DTYPE_MSCP		2		/* MSCP */
#define	DTYPE_DEC		3		/* other DEC (rk, rl) */
#define	DTYPE_SCSI		4		/* SCSI */
#define	DTYPE_ESDI		5		/* ESDI interface */
#define	DTYPE_ST506		6		/* ST506 etc. */
#define	DTYPE_HPIB		7		/* CS/80 on HP-IB */
#define	DTYPE_HPFL		8		/* HP Fiber-link */
#define	DTYPE_FLOPPY		10		/* floppy */

#ifdef DKTYPENAMES
static char *dktypenames[] = {
	"unknown",
	"SMD",
	"MSCP",
	"old DEC",
	"SCSI",
	"ESDI",
	"ST506",
	"HP-IB",
	"HP-FL",
	"type 9",
	"floppy",
	0
};
#define DKMAXTYPES	(sizeof(dktypenames) / sizeof(dktypenames[0]) - 1)
#endif

/*
 * Filesystem type and version.
 * Used to interpret other filesystem-specific
 * per-partition information.
 */
#define	FS_UNUSED	0		/* unused */
#define	FS_SWAP		1		/* swap */
#define	FS_V6		2		/* Sixth Edition */
#define	FS_V7		3		/* Seventh Edition */
#define	FS_SYSV		4		/* System V */
#define	FS_V71K		5		/* V7 with 1K blocks (4.1, 2.9) */
#define	FS_V8		6		/* Eighth Edition, 4K blocks */
#define	FS_BSDFFS	7		/* 4.2BSD fast file system */
#define	FS_MSDOS	8		/* MSDOS file system */
#define	FS_BSDLFS	9		/* 4.4BSD log-structured file system */
#define	FS_OTHER	10		/* in use, but unknown/unsupported */
#define	FS_HPFS		11		/* OS/2 high-performance file system */
#define	FS_ISO9660	12		/* ISO 9660, normally CD-ROM */
#define	FS_BOOT		13		/* partition contains bootstrap */

#ifdef	DKTYPENAMES
static char *fstypenames[] = {
	"unused",
	"swap",
	"Version 6",
	"Version 7",
	"System V",
	"4.1BSD",
	"Eighth Edition",
	"4.2BSD",
	"MSDOS",
	"4.4LFS",
	"unknown",
	"HPFS",
	"ISO9660",
	"boot",
	0
};
#define FSMAXTYPES	(sizeof(fstypenames) / sizeof(fstypenames[0]) - 1)
#endif
