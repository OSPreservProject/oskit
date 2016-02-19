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
 * This is a shared object. It is linked as an independent module that
 * is loaded into its own "address space" in an oskit kernel. All of
 * its external interfaces come through COM objects that are given to
 * it when the oskit kernel loads it. In other words, it is not linked
 * against the kernel when it is loaded, but contains its own C
 * library, Posix library, etc. The process runs in its own thread
 * context, although at this point the object itself is single threaded
 * since the kernel does not export the pthread interface. Signals don't
 * work either since the oskit kernel does not yet have a proper notion of
 * process context (and signals contain a lot "per-process" goop).
 */

#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>

/* dummy() is defined in a shared object. */
void	dummy(int);

/* Defined is dols.c */
void	dols(char *name, int lslong);

/* Simple network test */
int	resolve(char *b);

int
main(int argc, char **argv)
{
	int		i;
	void		(*func)(char *);
	void	       *handle;

	/*
	 * Print out our argument strings.
	 */
	for (i = 0; i < argc; i++) 
		printf("%s: arg:%d - %s\n", argv[0], i, argv[i]);

	/*
	 * Make sure dummy.so was loaded properly, since that was specified
	 * as a "NEEDED" object when the foo.so was linked. 
	 */
	dummy(69);

	/*
	 * Now load another .so file.
	 */
	if ((handle = dlopen("dopey.so", RTLD_NOW)) == NULL) {
		printf("dlopen failed: %s\n", dlerror());
		goto skip_dopey;
	}

	/*
	 * Find the symbol we want.
	 */
	if ((func = dlsym(handle, "dopey")) == NULL) {
		printf("dlsym failed: %s\n", dlerror());
		exit(1);
	}

	/*
	 * And call it.
	 */
	func("This is dopey");
	
	/*
	 * Lets unload it too.
	 */
	if (dlclose(handle) != NULL) {
		printf("dlclose failed: %s\n", dlerror());
		exit(1);
	}
 skip_dopey:
	
	/*
	 * Test out the filesystem interfaces.
	 */
	dols(".", 1);

	/*
	 * Test out the network interfaces.
	 */
	resolve("fast.cs.utah.edu");
	resolve("www.usps.gov");
	resolve("prep.ai.mit.edu");
	resolve("jackal");
	resolve("localhost");
	resolve("155.99.214.165");

	return 69;
}

#include <netdb.h>

int
resolve(char *b)
{
	struct hostent *he;
	printf("resolving %s...", b);
	fflush(stdout);
	he = gethostbyname(b);
	if (he) {
		printf("done.\n");
		printf("%s -> %s\n", b, inet_ntoa(*((long *)he->h_addr)));
		return 0;
	} else {
		printf("failed.\n");
		return -1;
	}
}
