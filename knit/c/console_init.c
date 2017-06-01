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

#include <oskit/x86/pc/base_console.h>
#include <oskit/x86/pc/gdb_console.h>
#include <oskit/gdb_serial.h> /* for gdb_serial_exit */
#include <oskit/x86/debug_reg.h>

/* This is a simple adaptor which lets base_console_init conform to
 * Knit's initialisation/finalisation requirements.
 */

extern int    boot_argc;
extern char** boot_argv;

oskit_error_t init(void)
{
        base_console_init(boot_argc,boot_argv);
        return 0;
}

oskit_error_t
fini(void)
{
        int rc;  /* ToDo: how can we get real value? */

        /* ToDo: make this part of the gdb finaliser! */
	if (enable_gdb) {
		/* Detach from the remote GDB. */
		gdb_serial_exit(rc);

#ifdef HAVE_DEBUG_REGS
		/* Turn off the debug registers. */
		set_dr7(get_dr7() & ~(DR7_G0 | DR7_G1 | DR7_G2 | DR7_G3));
#endif

	}

	/* flush and wait for `_exit called` message */
	if (console) {
            oskit_stream_release(console);
            console = 0;
        }
        return 0;
}

