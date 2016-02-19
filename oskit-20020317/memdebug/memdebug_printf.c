/*
 * Copyright (c) 1996, 1998 University of Utah and the Flux Group.
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
 * Provide a ``safe'' printf.  We want to avoid printf's that cause
 * memory allocations, the oskit default printf() is static, but
 * client OSes may not be.  The can override this as necessary.
 */
#include <stdarg.h>
#include <stdio.h>

#include "memdebug.h"

int
memdebug_printf(const char *fmt, ...)
{
        va_list args;
        int err;

        va_start(args, fmt);
        err = vprintf(fmt, args);
        va_end(args);

        return err;
}

