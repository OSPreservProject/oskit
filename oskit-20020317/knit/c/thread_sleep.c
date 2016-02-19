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

/* 
 * This is the initializer for the threaded version of osenv_sleep.
 * The idea is that you can't use osenv_sleep unless you're in a thread
 * so this puts you in a thread.
 *
 * Not sure if this is the right thing to do - but it's worth a shot.
 */

#include <oskit/error.h>
#include <threads/pthread_internal.h>

oskit_error_t init_sleep(void)
{
        osenv_process_lock();
        return 0;
}

oskit_error_t fini_sleep(void)
{
        /* probably not needed */
        osenv_process_unlock();
        return 0;
}
