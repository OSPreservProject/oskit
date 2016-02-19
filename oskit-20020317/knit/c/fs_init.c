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
#include <oskit/fs/filesystem.h>
#include <oskit/dev/dev.h>

extern oskit_blkio_t *blkio;
extern int read_only;
oskit_filesystem_t *fs = 0;

oskit_error_t init(void)
{
        oskit_filesystem_t *unwrapped_fs = 0;
        int rc;

        osenv_process_lock();
        rc = fs_netbsd_mount(blkio, 
                             read_only ? OSKIT_FS_RDONLY : 0, 
                             &unwrapped_fs);
        osenv_process_unlock();
        if (rc) return rc;

        /* Note that these casts change the number of arguments to lock/unlock */
        rc = oskit_wrap_filesystem(unwrapped_fs,
                                   (void (*)(void *))osenv_process_lock,
                                   (void (*)(void *))osenv_process_unlock,
                                   0, &fs);
        oskit_filesystem_release(unwrapped_fs);
        return rc;
}

oskit_error_t fini(void)
{
        if (fs) {
                /* 
                 * Note that this can go into an infinite loop if the
                 * process_lock is held when this is called.
                 * Not sure what the best fix is.
                 */
                oskit_filesystem_sync(fs, 1);
                oskit_filesystem_release(fs);
                fs = 0;
        }
        return 0;
}
