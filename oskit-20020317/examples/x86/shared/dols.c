/*
 * Copyright (c) 1997-1998 University of Utah and the Flux Group.
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
 * a hacked up ls, tests opendir/readdir/rewinddir/closedir, chdir, fchdir
 * and open, stat, and lstat
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#ifndef VERBOSITY
#define VERBOSITY 	0
#endif

void
printinfo(struct stat *stbuf, char *name)
{
	extern char *ctime(const time_t *);

        char    *p, type;
        switch (stbuf->st_mode & S_IFMT) {
        case S_IFDIR:
                type = 'd';
                break;
        case S_IFLNK:
                type = 'l';
                break;
        case S_IFBLK:
                type = 'b';
                break;
        case S_IFCHR:
                type = 'c';
                break;
        case S_IFREG:
                type = '-';
                break;
        default:
                type = '?';
        }
        p = ctime(&stbuf->st_mtime);
        p[24] = 0;

	/* XXX do S_ISUID, S_ISGID, S_ISTXT */
        printf("%c%c%c%c%c%c%c%c%c%c %8d\t%s\t%s%c", type,
		(stbuf->st_mode & S_IRUSR) ? 'r' : '-',
		(stbuf->st_mode & S_IWUSR) ? 'w' : '-',
		(stbuf->st_mode & S_IXUSR) ? 'x' : '-',
		(stbuf->st_mode & S_IRGRP) ? 'r' : '-',
		(stbuf->st_mode & S_IWGRP) ? 'w' : '-',
		(stbuf->st_mode & S_IXGRP) ? 'x' : '-',
		(stbuf->st_mode & S_IROTH) ? 'r' : '-',
		(stbuf->st_mode & S_IWOTH) ? 'w' : '-',
		(stbuf->st_mode & S_IXOTH) ? 'x' : '-',
		(int)stbuf->st_size, p, name,
		type =='d' ? '/' : ' ');

	if ((stbuf->st_mode & S_IFMT) == S_IFLNK) {
		static char buf[1024];
		int 	l = readlink(name, buf, sizeof buf);

		if (-1 == l)
			printf("-> ???");
		else {
			/* readlink doesn't append a terminating NUL char ! */
			buf[l] = 0;
			printf("-> %s", buf);
		}
	}
	printf("\n");
}

void
dols(char *name, int lslong)
{
        struct stat stbuf;
#if VERBOSITY > 1
	printf(__FUNCTION__"(`%s',%d) called\n", name, lslong);
#endif
        lstat(name, &stbuf);

        switch (stbuf.st_mode & S_IFMT) {
        case S_IFDIR:
        {
		DIR	*fd;
                struct dirent *de;
		int 	thisdir = open(".", O_RDONLY);

		if (thisdir == -1) {
			perror("open .");
			printf(__FUNCTION__": couldn't open current dir, "
				"errno = x%x\n", errno);
			return;
		}

		if (-1 == chdir(name)) {
			perror(name);
			printf(__FUNCTION__": chdir(`%s') failed\n", name);
			return;
		}

		/* open directory */
		fd = opendir(".");
		if (fd == 0) {
			perror("opendir .");
			printf("%s: no such file or directory\n", name);
			return;
		}

		/* iterate through directory and print items */
		while ((de = readdir(fd)) != 0) {
			if (-1 == lstat(de->d_name, &stbuf)) {
				perror("lstat");
				printf(__FUNCTION__": lstat(`%s') failed\n",
					de->d_name);
				return;
			}
			if (lslong)
				printinfo(&stbuf, de->d_name);
			else
				printf("%s%c\n", de->d_name,
					S_ISDIR(stbuf.st_mode) ? '/' : ' ');
		}

		rewinddir(fd);

		while ((de = readdir(fd)) != 0) {
			char buf[512];

			if (-1 == lstat(de->d_name, &stbuf)) {
				perror("lstat");
				printf(__FUNCTION__": stat de->d_name=`%s' "
						"failed\n", de->d_name);
				break;
			}
			if (S_ISDIR(stbuf.st_mode)) {
				strcpy(buf, de->d_name);
				if (strcmp(buf, "..") && strcmp(buf, ".")) {
					printf("\n%s:\n", buf);
					dols(buf, lslong);
				}
			}
		}
		closedir(fd);

		/* change back */
		if (-1 == fchdir(thisdir)) {
			perror("fchdir");
			return;
		}
		close(thisdir);
                break;
        }
        case S_IFREG:
        case S_IFLNK:
                if (lslong)
                        printinfo(&stbuf, name);
                else
                        printf("%s\n", name);
                break;
        default:
                printf("unknown type = x%x, DIR= x%x, REG= x%x\n",
                        stbuf.st_mode, S_IFDIR, S_IFREG);
        }
}


#ifdef TEST_ME_IN_UNIX
main(int ac, char *av[])
{
	dols(ac > 1 ? av[1] : ".", 1);
	return 0;
}
#endif
