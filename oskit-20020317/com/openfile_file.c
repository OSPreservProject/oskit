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

#include <oskit/fs/soa.h>

OSKIT_COMDECL 
oskit_soa_openfile_from_file(oskit_file_t *f, 
			    oskit_oflags_t flags,
			    struct oskit_openfile **out_openfile)
{
	oskit_absio_t *aio;
	oskit_stream_t *str;
	int rc;

	if (oskit_file_query(f, &oskit_absio_iid, (void **) &aio) == 0) {
		rc = oskit_soa_openfile_from_absio(f, aio, flags, out_openfile);
		oskit_absio_release(aio);
	} else if (oskit_file_query(f, &oskit_stream_iid, (void **) &str) == 0) {
		rc = oskit_soa_openfile_from_stream(f, str, flags, out_openfile);
		oskit_stream_release(str);
	} else
		rc = oskit_soa_openfile_from_stream(f, 0, flags, out_openfile);
	return rc;
}

