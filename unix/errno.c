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

#include <stdlib.h>

#include <oskit/dev/dev.h> /* osenv_log */

#include "native.h"
#include "native_errno.h"

#if OSKITUNIX_USE_ERRNOTABLE
/*
 * Define a table that maps native errnos to the
 * corresponding OSKit errno.  Takes advantage of
 * GNUC array initialization magic.
 */
const oskit_error_t oskitunix_native_errors[] = {
#define NATIVE_ERROR(native, oskit) [native] = oskit,
#include "native_errnos.h"
};
const int oskitunix_native_nerrors =
	(sizeof(oskitunix_native_errors) / sizeof(oskitunix_native_errors[0]));

#endif /* OSKITUNIX_USE_ERRNOTABLE */

/**
 * Perror which looks for errno in NATIVEOS(errno).
 */
void
oskitunix_perror(const char *msg)
{
	const char* strerr;

	strerr = strerror(native_to_oskit_error(NATIVEOS(errno)));

	/* empty messages get no prefix at all */
	if ((msg == NULL) || (msg[0] == '\0')) {
		osenv_log(OSENV_LOG_ERR, "%s\n", strerr);
	} else {
		osenv_log(OSENV_LOG_ERR, "%s: %s\n", msg, strerr);
	}
}

