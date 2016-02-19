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

#include <oskit/dev/dev.h>
#include <oskit/com/wrapper.h>

oskit_blkio_t*
osenv_wrap_blkio(oskit_blkio_t *bio)
{
        oskit_error_t	err;
        oskit_blkio_t	*wrappedbio;
		
        /*
         * This test lets you write mildly sloppy code like this:
         *
         *    oskit_wrap_blkio_sleep(get_blkio(...))
         *
         */
        if (bio == 0)
                return 0;

        /*
         * Wrap the partition blkio.
         */
        err = oskit_wrap_blkio(bio,
			       (void (*)(void *))osenv_process_lock, 
			       (void (*)(void *))osenv_process_unlock,
			       0, &wrappedbio);
        if (err)
                return 0;
        
        /*
         * Don't need the partition anymore, the wrapper has a ref.
         */
        oskit_blkio_release(bio);

        return wrappedbio;
}

