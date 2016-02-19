/*
 * Copyright (c) 1998, 1999 The University of Utah and the Flux Group.
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
 * Posix library internal definitions.
 */

#ifndef _OSKIT_POSIX_H_
#define _OSKIT_POSIX_H_

#include <oskit/fs/dir.h>
#include <oskit/dev/osenv_intr.h>

/*
 * Initialize the POSIX library.
 */
void		oskit_posixlib_init(void);

/*
 * Initialize the system clock. Return status.
 */
int		posixlib_clock_init(void);

/*
 * Cleanup all the open descriptors.
 */
void		fd_cleanup(void);

/*
 * Some internal functions that interface to the filesystem namespace.
 */
oskit_error_t	fs_chdir(oskit_dir_t *dir);
oskit_error_t	fs_chroot(const char *path);

#ifndef KNIT
/*
 * The oskit_interrupt object, which provides the hooks to interrupt
 * enable/disable in both single and multi-threaded mode. 
 */
extern oskit_osenv_intr_t	*posixlib_osenv_intr_iface;
void		posixlib_osenv_intr_init_iface(void);
#endif

#endif /* _OSKIT_STARTUP_H_ */
