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

#include <stdlib.h> /* atexit, exit */

#ifdef __ELF__
extern void __oskit_init(void);
extern void __oskit_fini(void);
#endif

#ifdef GPROF
extern int enable_gprof;
extern void base_gprof_init();
#endif

extern int simplemain(void);
extern char **environ;
extern char **prog_argv;
extern int    prog_argc;


void invoke_main()
{
#ifdef GPROF
	if (enable_gprof)
		base_gprof_init();
#endif

#ifdef __ELF__
	/* Make sure deinit code gets called on exit. */
	atexit(__oskit_fini);

	/* Call init code. */
	__oskit_init();
#endif

	/* Invoke the main program. */
	exit(simplemain());
}

