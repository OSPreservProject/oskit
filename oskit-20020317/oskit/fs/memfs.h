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

/*
 * this implements a simple boot module file system - just call
 * oskit_memfs_init() to obtain a filesystem.
 * If you want to copy in your bmod files into the bmodfs, then
 * call start_fs_bmod(), which does the work for you.  It also
 * returns a directory to you which is the root of your bmod.
 */
#include <oskit/fs/dir.h>
#include <oskit/fs/filesystem.h>
#include <oskit/dev/osenv.h>
#include <oskit/dev/osenv_log.h>
#include <oskit/dev/osenv_mem.h>

#ifndef _MEMFS_H_
#define _MEMFS_H_

/*
 * Initialize the memory filesystem, and return a handle to
 * the filesystem.  Note that you'll need to populate
 * this filesystem manually.  The start_fs_bmod()
 * call will copy your bmods into the filesystem for you.
 */
oskit_error_t oskit_memfs_init(
#ifndef KNIT
                                oskit_osenv_t *osenv,
#endif
				oskit_filesystem_t **out_fs);

/*
 * New entry point which makes dependencies explicit.
 */
oskit_error_t oskit_memfs_init_ex(oskit_osenv_mem_t   *mem,
				  oskit_osenv_log_t   *log,
				  oskit_filesystem_t **out_fs);

/*
 * This function replaces a bmod file's contents with the given data pointer.
 * `file' must be a file in a bmod filesystem, and not a directory.
 * `size' gives the new size of the file, `allocsize' the total amount writable
 * from `data'.  If `inhibit_resize' is TRUE, normal attempts to change
 * the size of the file hereafter will fail with OSKIT_EPERM; only another
 * call to this function can change it.  If `can_sfree' is TRUE,
 * `sfree(data, allocsize);' is called if the file grows beyond `allocsize';
 * otherwise, the original data pointer is just leaked.
 */
oskit_error_t oskit_memfs_file_set_contents(oskit_file_t *file,
					    void *data, oskit_off_t size,
					    oskit_off_t allocsize,
					    oskit_bool_t can_sfree,
					    oskit_bool_t inhibit_resize);


#endif /* _MEMFS_H_ */

