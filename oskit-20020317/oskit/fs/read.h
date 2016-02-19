/*
 * Copyright (c) 1997-1998 University of Utah and the Flux Group.
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
 * Definitions for the OSKIT's file system reader modules.
 * These are simple, minimal file system interpreters
 * suitable for reading files off of a disk during bootstrap or whatnot.
 * They are NOT general-purpose file system implementations by far,
 * in that they don't support writes, multithreading, concurrent I/O,
 * permission checking, or other fancy things like that.
 */
#ifndef _OSKIT_FS_READ_H_
#define _OSKIT_FS_READ_H_

#include <oskit/com.h>
#include <oskit/io/blkio.h>

/*
 * Simple file-open interfaces to the individual file system readers.
 * Each of these functions takes a blkio interface to the underlying device
 * and a pathname relative to the root directory of the file system,
 * and if the specified file can be found,
 * returns a blkio object that can be used to read from that file.
 * The blkio object returned will have a block size of 1,
 * meaning that there are no alignment restrictions on file reads.
 * The blkio object passed, representing the underlying device,
 * can have a block size greater than 1,
 * but if it is larger than the file system's block size,
 * file system interpretation will fail.
 * Also, any absolute symlinks followed during the open
 * will be interpreted as if this is the root file system.
 */
oskit_error_t fsread_ffs_open(oskit_blkio_t *device, const char *path,
			      oskit_blkio_t **out_file);
oskit_error_t fsread_ext2_open(oskit_blkio_t *device, const char *path,
			       oskit_blkio_t **out_file);
oskit_error_t fsread_minix_open(oskit_blkio_t *device, const char *path,
			        oskit_blkio_t **out_file);

/*
 * This function is simply a wrapper for the above functions:
 * it tries each supported file system type in turn
 * until one of them succeeds.
 */
oskit_error_t fsread_open(oskit_blkio_t *device, const char *path,
			  oskit_blkio_t **out_file);

#if 0
/* This does not exist yet. */
/*
 * This function provides an additional convenience layer over fsread_open:
 * given a reference to the underlying device on which a file system resides,
 * it detects the file system type, opens the file system's root directory,
 * and uses appropriate interface adaptors
 * to provide a OSKit COM file system interface
 * which can be traversed or mounted into a larger file system structure.
 */
oskit_error_t fsread_getfs(struct blkio *device,
			  struct oskit_dir **out_root_dir);
#endif

#endif /* _OSKIT_FS_READ_H_ */
