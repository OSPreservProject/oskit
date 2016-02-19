/*
 * Copyright (c) 2000 University of Utah and the Flux Group.
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

#ifndef UNIX_NATIVE_ERRNO_H
#define UNIX_NATIVE_ERRNO_H

#include <oskit/error.h>
#include <errno.h>

/* A perror implementation that uses NATIVEOS(errno) */
void oskitunix_perror(const char *msg);

/**
 * Use EINVAL to determine if a errno mapping table
 * is used or if a dynamic switch statement is used to map
 * native errno numbers into OSKit errno numbers.
 *
 * I'm pretty sure both EINVAL and 1000 were just pulled out of the
 * air.
 */
#define OSKITUNIX_USE_ERRNOTABLE (EINVAL < 1000)

extern int native_errno; /* XXX NATIVEOS(errno) */

#if OSKITUNIX_USE_ERRNOTABLE
extern const oskit_error_t oskitunix_native_errors[];
extern const int oskitunix_native_nerrors;
#endif

static inline oskit_error_t
native_to_oskit_error(const int err)
{
#if OSKITUNIX_USE_ERRNOTABLE
	assert(oskitunix_native_nerrors > 0);
	if ((unsigned)err < oskitunix_native_nerrors)
		return oskitunix_native_errors[err];
#else
	switch (err) {
#define NATIVE_ERROR(native, oskit) case native: return oskit;
#include "native_errnos.h"
#undef NATIVE_ERROR
	}
#endif
	return OSKIT_E_FAIL;
}

#endif /* UNIX_NATIVE_ERRNO_H */
