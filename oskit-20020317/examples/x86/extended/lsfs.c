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
 * The trivial ls kernel ...
 */

#define DISK_NAME	"wd1"
#define PARTITION_NAME	"b"
#define DIRECTORY_NAME	"/"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <oskit/clientos.h>
#include <oskit/startup.h>

/* Defined is dols.c */
void	dols(char *name, int lslong);

void
main(argc, argv)
	int	argc;
	char	*argv[];
{
	char		*option, ch;
	int		fs_long    = 0;
	char		*diskname  = DISK_NAME;
	char		*partname  = PARTITION_NAME;
	extern char	*optarg;
	extern int	optind;
	void		usage(void);

	if ((option = getenv("DISK")) != NULL)
		diskname = option;
	
	if ((option = getenv("PART")) != NULL)
		partname = option;

	/*
	 * Create the Client OS. 
	 */
	oskit_clientos_init();
	start_clock();
#ifndef OSKIT_UNIX
	start_fs(diskname, partname);
#else
#ifdef  REALSTUFF
	start_fs("/dev/oskitfs", 0);
#else
	start_fs_native("/");
#endif
#endif
	while ((ch = getopt(argc, argv, "l")) !=  -1) {
		switch (ch) {
		case 'l':
			fs_long++;
			break;

		default:
		case '?':
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (argc) {
		while (argc) {
			dols(argv[0], fs_long);
			argv++;
			argc--;
		}
	}
	else 
		dols(DIRECTORY_NAME, fs_long);
	
	exit(0);
}

void
usage()
{
	(void)fprintf(stderr, "usage: lsfs [-l] [file ...]\n");
	exit(1);
}
