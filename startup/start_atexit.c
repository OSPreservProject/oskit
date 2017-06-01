/*
 * Copyright (c) 1997-1999 University of Utah and the Flux Group.
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
 * The point of this code is to ensure that the atexit routines that are
 * needed to clean up the startup library, get run as the very last
 * atexit, and as a block. Relying on libc atexit has the undesirable effect
 * of scattering the startup library atexits around, and possibly even causing
 * them to run before all the libc atexits run.
 */

#include <oskit/c/stdio.h>
#include <oskit/c/stdlib.h>
#include <oskit/c/unistd.h>
#include <oskit/startup.h>

typedef void (*atexit_function_ptr)(void *arg);

struct startup_atexit_func {
	atexit_function_ptr	func;
	void		        *arg;
}; 

static struct startup_atexit_func startup_atexit_functions[16];
static int		 startup_atexit_index = 0;
static void		 run_startup_atexits(void);
static int		 startup_atexit_ready;

void
startup_atexit_init(void)
{
	if (! startup_atexit_ready) {
		atexit(run_startup_atexits);
		startup_atexit_ready = 1;
	}
}

int
startup_atexit(void (*function)(void *arg), void *arg)
{
	if (!startup_atexit_ready)
		startup_atexit_init();
	
	/*
	 * We support only 16 atexit functions.
	 */
	if (startup_atexit_index >= 16)
		return -1;

	startup_atexit_functions[startup_atexit_index].func = function;
	startup_atexit_functions[startup_atexit_index].arg  = arg;
	startup_atexit_index++;
	return 0;
}

/*
 * Call atexit functions in reverse order.
 */
static void
run_startup_atexits(void)
{
	int i;

	for (i = startup_atexit_index - 1; i >= 0; i--) {
		atexit_function_ptr	func;
		void		        *arg;

		func = startup_atexit_functions[i].func;
		arg  = startup_atexit_functions[i].arg;

		func(arg);
	}
}

