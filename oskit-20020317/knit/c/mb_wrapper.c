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

#include <oskit/types.h>
#include <oskit/error.h>
#include <oskit/x86/pc/reset.h>

oskit_error_t knit_init_0(void);
oskit_error_t knit_init_1(void);
oskit_error_t knit_init_2(void);
oskit_error_t knit_init_3(void);
void multiboot_main_wrapped(oskit_addr_t boot_info_pa);
void invoke_main(void);

void multiboot_main(oskit_addr_t boot_info_pa)
{
        int rc;
	multiboot_main_wrapped(boot_info_pa);
        if ((rc=knit_init())) _exit(rc);
        invoke_main();
        /* call me paranoid... */
        _exit(1);
}

static int exiting = 0;

void _exit(int rc)
{
        // Sometimes the exit code panics which causes an infinite loop.
        // The real fix (?) is to pay proper attention to the panic 
        // dependency in finalisers...
        if (!exiting) {
                exiting = 1;
                knit_fini();
        }
        pc_reset();
}

