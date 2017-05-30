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
 * This demonstrates the POSIX fs interfaces and the memory filesystem.
 *
 * For this example to work, you need to include a bmod
 * in your bootimage, this can be accomplished by
 *
 * 	mkbsdimage this_file some_text_file:whatever
 *
 * The expected output is the content of `some_text_file'.
 */

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <oskit/fs/memfs.h>
#include <oskit/c/fs.h>
#include <assert.h>
#include <oskit/clientos.h>
#include <oskit/startup.h>

void
main()
{
	char	buf[64];
	int 	r, fd;
	DIR	*d;
	char 	fname[256];
	struct dirent *de = 0;
	extern void dols(char *path, int long);

#ifndef KNIT
	oskit_clientos_init();
#ifdef OSKIT_UNIX
	/*
	 * In unix mode, be sure to setenv your OSKITROOT variable,
	 * if you want your memfs populated.
	 */
	start_fs_native_bmod();
#else
	start_fs_bmod();
#ifdef  GPROF
	start_gprof();
#endif
#endif
#endif /* !KNIT */

	printf("---Listing root dir\n");
	dols("/", 1);

	/* now open it and look for the first real entry */
	d = opendir("/");
	if (!d) {
		perror("opendir");
		exit(0);
	}

	while ((de = readdir(d)) != 0)
		if (strcmp(de->d_name, ".") &&
		    strcmp(de->d_name, ".."))
			break;

	/*
	 * read the file
	 */
	if (!de) {
		printf("No bmods supplied!\n");
		exit(0);
	}
	sprintf(fname, "/%s", de->d_name);
	closedir(d);

	printf("\n---Reading `%s'\n", fname);

	fd = open(fname, O_RDONLY);
	if (fd == -1) {
		perror(fname);
		exit(0);
	}

	while ((r = read(fd, buf, sizeof buf)) > 0)
		write(1, buf, r);
	exit(0);
}
