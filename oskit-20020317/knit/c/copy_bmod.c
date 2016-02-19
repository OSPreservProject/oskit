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

#include <oskit/io/blkio.h>
#include <oskit/fs/netbsd.h>
#include <oskit/startup.h>

extern oskit_filesystem_t *in_fs;
oskit_filesystem_t        *out_fs = 0;

oskit_error_t init(void)
{
    oskit_dir_t *root;
    int rc;

    rc = oskit_filesystem_getroot(in_fs, &root);
    if (rc) 
        return rc;
    rc = bmod_populate(root,0,0);
    oskit_dir_release(root);
    out_fs = in_fs;  /* bit of a hack that out = in... */
    return rc;
}

oskit_error_t
fini(void)
{
        /* ToDo: ??? */
    return 0;
}
