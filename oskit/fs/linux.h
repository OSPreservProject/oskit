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

#ifndef _OSKIT_FS_LINUX_H_
#define _OSKIT_FS_LINUX_H_

#include <oskit/io/blkio.h>
#include <oskit/fs/filesystem.h>
#include <oskit/compiler.h>

OSKIT_BEGIN_DECLS

extern oskit_error_t fs_linux_init(void);

extern oskit_error_t fs_linux_mount(oskit_blkio_t *bio,
				   oskit_u32_t flags,
				   oskit_filesystem_t **out_fs);

OSKIT_END_DECLS

#endif /* _OSKIT_FS_LINUX_H_ */
