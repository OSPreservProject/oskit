/*
 * Copyright (c) 1999 University of Utah and the Flux Group.
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
 * Simple demonstration kernel showing how to use the Runtime Loader.
 */
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>

#include <oskit/threads/pthread.h>
#include <oskit/startup.h>
#include <oskit/clientos.h>

/* #define NEWSTUFF */

void main(int argc, char **argv)
{
	void	       *handle;
	void		(*func)(int);
#ifdef  NEWSTUFF
	pthread_t	tid;
	char		*newargv[3];
	void		*status;
	int		rc;
	extern char   **environ;
#endif	
	/*
	 * Init the OS.
	 */
	oskit_clientos_init_pthreads();
	start_world_pthreads();

	printf("ClientOS ready\n");

	/*
	 * Set our LD path. The startup code is going to drop the contents
	 * of the bmod in /bmod, so thats where the RTLD should look.
	 */
	setenv("LD_LIBRARY_PATH", "/bmod", 1);

	/*
	 * Must initialize the RTLD library. 
	 */
	oskit_boot_rtld(NULL);

	/*
	 * First lets load a shared object into the kernel's "address space."
	 */
	if ((handle = dlopen("dummy.so", RTLD_NOW)) == NULL) {
		printf("dlopen failed: %s\n", dlerror());
		exit(1);
	}

	/*
	 * Find the symbol we want.
	 */
	if ((func = dlsym(handle, "dummy")) == NULL) {
		printf("dlsym failed: %s\n", dlerror());
		exit(1);
	}

	/*
	 * And call it.
	 */
	func(96);

	/*
	 * Lets unload it too.
	 */
	if (dlclose(handle) != NULL) {
		printf("dlclose failed: %s\n", dlerror());
		exit(1);
	}

#ifdef  NEWSTUFF
	/*
         ******************************************************************
	 * Try to "spawn" another shared object into a new region.
	 * That is, it is not linked against the oskit kernel, but is
	 * a entirely self contained program.
	 */
	if (argc == 2) {
		newargv[0] = argv[1];
	}
	else
		newargv[0] = "foo.so";
		
	newargv[1] = "Hi There";
	newargv[2] = "Bye There";
	newargv[3] = 0;

	printf("Spawning %s\n", newargv[0]);

	if ((rc = oskit_spawn(&tid, newargv[0], newargv, environ)) != 0) {
		printf("Oops, oskit_spawn failed with 0x%x\n", rc);
		exit(1);
	}

	printf("pthread_spawn returned! Waiting for child to exit\n");

	pthread_join(tid, &status);

	printf("pthread_join returned %p\n", status);
#endif
	exit(0);
}

