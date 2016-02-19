/*
 * Copyright (c) 1997-1999 University of Utah and the Flux Group.
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
/* This file lists the filesystems to be included in liboskit_linux_fs.
 * Each line is of the form:
 *
 *	filesystem(dirname, description, initfunction)
 *
 * dirname	is the name of the directory where the source
 *		lives, e.g., ext2;
 * description	is a brief description of the filesystem like that
 *		in Linux's fs/Config.in;
 * initfunction is the name of the initialization function,
 *		which is usually init_<dirname>_fs but not always
 *		(for example iso9660).
 *
 * NOTE: this is processed by cpp and by awk.
 * This means that if you want to "comment" one of these out you must
 * make it invisible to cpp and make it not match /^filesystem/.
 * So you can make unwanted lines be comment lines or #if 0 them out and
 * indent them a space.
 */

filesystem(ufs, "UFS filesystem", init_ufs_fs)
filesystem(ext2, "Second extended filesystem", init_ext2_fs)
filesystem(minix, "Minix filesystem", init_minix_fs)
filesystem(vfat, "VFAT (Win95) filesystem", init_vfat_fs)
filesystem(msdos, "MSDOS filesystem", init_msdos_fs)
filesystem(fat, "DOS FAT filesystem", init_fat_fs)
filesystem(isofs, "ISO9660 CDROM filesystem", init_iso9660_fs)
filesystem(sysv, "System V and Coherent filesystems", init_sysv_fs)
filesystem(hpfs, "OS/2 HPFS filesystem (read only)", init_hpfs_fs)
filesystem(affs, "Amiga FFS filesystem", init_affs_fs)
/* filesystem(ntfs, "WinNT NTFS filesystem support (read only)", init_ntfs_fs) */
