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
 * The Client OS interface. Used to set up the OS environment.
 */
#ifndef _OSKIT_CLIENTOS_H_
#define _OSKIT_CLIENTOS_H_

#include <oskit/com.h>
#include <oskit/error.h>
#include <oskit/dev/clock.h>
#include <oskit/dev/osenv_intr.h>
#include <oskit/dev/osenv_sleep.h>
#include <oskit/com/stream.h>
#include <oskit/io/ttystream.h>
#include <oskit/fs/fsnamespace.h>
#include <oskit/com/libcenv.h>

/*
 * Entrypoints for initializing the C library dependencies of the
 * client OS.
 */
#define oskit_clientos_sethostname(name, len) \
	oskit_libcenv_sethostname(initial_clientos_libcenv, (name), (len))

#define oskit_clientos_setconsole(ttystrm) \
	oskit_libcenv_setconsole(initial_clientos_libcenv, (ttystrm))

#define oskit_clientos_setfsnamespace(fsn) \
	oskit_libcenv_setfsnamespace(initial_clientos_libcenv, (fsn))

/*
 * Initialize the client OS data structures.
 */
oskit_error_t		oskit_clientos_init(void);
oskit_error_t		oskit_clientos_init_pthreads(void);

#endif /* _OSKIT_CLIENTOS_H_ */

