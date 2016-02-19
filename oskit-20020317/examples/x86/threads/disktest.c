/*
 * Copyright (c) 1996, 1998, 1999, 2000 University of Utah and the Flux Group.
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
 * Disk Thrasher: This example is intended to demonstrate a multi-threaded
 * program that uses the BSD file layer. Its basically a disk thrasher; a
 * whole bunch of threads are started. Each one creates a big file of random
 * numbers, and then repeatedly copies it to a destination file, and then
 * rewinds each and compares the two files to make sure it was copied
 * correctly. Each thread writes in different sized chunks. Both files are
 * closed and re-opened each time through the loop.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <ctype.h>
#include <strings.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <oskit/threads/pthread.h>
#include <oskit/clientos.h>
#include <oskit/startup.h>
#include <assert.h>
#include <sys/stat.h>
#include <oskit/queue.h>
#include <time.h>

#include <oskit/dev/dev.h>
#include <oskit/dev/linux.h>
#ifdef  OSKIT_UNIX
void	start_fs_native_pthreads(char *root);
#endif
int	dirinit();
int	count, activity;

#define	MAXTESTERS	3
#define TESTDIR		"/testdir"
#define TESTBYTES	50000

int	workers		= MAXTESTERS;
int     testbytes	= TESTBYTES;

int
main()
{
	pthread_t	foo[1024], bar;
	void		*stat;
	void		*worker(void *arg);
	void		*checker(void *arg);
	int		i;
	char		*option;
	struct timeval  before, after, diff;

#ifndef KNIT
	/*
	 * Init the OS.
	 */
	oskit_clientos_init_pthreads();
	
#ifndef OSKIT_UNIX
	{
		extern int fs_netbsd_nbuf;
		extern int fs_netbsd_doclusterread;
		extern int fs_netbsd_doclusterwrite;
		extern int fs_netbsd_thread_count;

		fs_netbsd_nbuf = 256;
		fs_netbsd_doclusterread = 0;
		fs_netbsd_doclusterwrite = 0;

		if ((option = getenv("FSTHREADS")) != NULL) {
			int fsthreads = atoi(option);
			fs_netbsd_thread_count = fsthreads;
		}
	}
	setenv("no-network", "yep", 1);
	start_world_pthreads();
#else /* !OSKIT_UNIX */
	{
		start_clock();
		start_pthreads();
#ifdef  REALSTUFF
		start_fs_pthreads("/dev/oskitfs", 0);
#else
		start_fs_native_pthreads("/tmp");
#endif
	}
#endif /* !OSKIT_UNIX */
#endif /* !KNIT */

	if ((option = getenv("WORKERS")) != NULL)
		workers = atoi(option);
	
	if ((option = getenv("TESTSIZE")) != NULL)
		testbytes = atoi(option);

	printf("Running with %d workers, %d byte files.\n", workers, testbytes);

	srand(1);
	dirinit();

	gettimeofday(&before, 0);
	count = workers;
	for (i = 0; i < workers; i++)
		pthread_create(&foo[i], 0, worker, (void *) i);

	pthread_create(&bar, 0, checker, (void *) 0);

	for (i = 0; i < workers; i++) {
		pthread_join(foo[i], &stat);
		count--;
	}
	gettimeofday(&after, 0);
	timersub(&after, &before, &diff);
	printf("Disk Tester took %d:%d seconds\n", diff.tv_sec, diff.tv_usec);
	
	return 0;
}

void *
worker(void *arg)
{
	int	i = (int) arg;
	int	srcfd, dstfd, loop, c, chunksize, count, err;
	char	srcname[128], dstname[128];
	char	*buf1, *buf2;
	struct stat statb;

	chunksize = 2048 + (i * 256);

	printf("Worker %d starting: Chunksize = %d bytes\n", i, chunksize);

	if ((buf1 = malloc(chunksize)) == NULL)
		panic("worker %d: Not enough memory\n", i);

	if ((buf2 = malloc(chunksize)) == NULL)
		panic("worker %d: Not enough memory\n", i);

	sprintf(srcname, "%s/%d/srcfile", TESTDIR, i);
	sprintf(dstname, "%s/%d/dstfile", TESTDIR, i);
	
	/*
	 * Open up the src file and stick some junk in it.
	 */
	if ((srcfd = open(srcname, O_RDWR|O_CREAT, 0777)) < 0)
		panic("worker %d: Could not open src file %s\n", i, srcname);

	count = 0;
	while (count < testbytes) {
		count += chunksize;
		activity = 1;

		for (c = 0; c < chunksize; c++)
			buf1[c] = rand() % 169;
		
		if ((err = write(srcfd, buf1, chunksize)) != chunksize) {
			if (err < 0) {
				perror("write");
				exit(1);
			}
			else {
				printf("write error");
				exit(2);
			}
		}
	}
	close(srcfd);

	/*
	 * Now loop!
	 */
	for (loop = 0; loop < 10; loop++) {
		printf("Worker %d, Loop %d\n", i, loop);
			
		/*
		 * Open up the src and dst files.
		 */
		if ((srcfd = open(srcname, O_RDONLY, 0777)) < 0) {
			printf("worker %d: opening src file %s\n", i, srcname);
			pthread_exit(0);
		}

		if ((dstfd = open(dstname,
				  O_RDWR|O_CREAT|O_TRUNC, 0777)) < 0) {
			printf("worker %d: opening dst file %s\n", i, dstname);
			close(srcfd);
			pthread_exit(0);
		}

		/*
		 * Lets stat it just for the hell of it.
		 */
		if (stat(dstname, &statb) < 0) {
			perror("stat ");
			printf("worker %d:\n", i);
			close(srcfd);
			close(dstfd);
			pthread_exit(0);
		}

		/*
		 * Copy src to destination in the chunksize.
		 */
		while (1) {
			activity = 1;
			if ((c = read(srcfd, buf1, chunksize)) <= 0) {
				if (c < 0) {
					perror("read ");
					printf("worker %d:\n", i);
					close(srcfd);
					close(dstfd);
					pthread_exit(0);
				}
				break;
			}
			if ((c = write(dstfd, buf1, c)) <= 0) {
				if (c < 0) {
					perror("write ");
					printf("worker %d:\n", i);
					close(srcfd);
					close(dstfd);
					pthread_exit(0);
				}
				break;
			}
		}
		
		/*
		 * Check length.
		 */
		c = (int) lseek(srcfd, 0, SEEK_END);
		if (c != count)
			panic("SRC file is the wrong size: %d bytes\n", c);
			
		c = (int) lseek(dstfd, 0, SEEK_END);
		if (c != count)
			panic("DST file is the wrong size: %d bytes\n", c);

		/*
		 * Rewind and compare the files.
		 */
		lseek(srcfd, 0, SEEK_SET);
		lseek(dstfd, 0, SEEK_SET);

		while (1) {
			activity = 1;
			if ((c = read(srcfd, buf1, chunksize)) <= 0) {
				if (c < 0) {
					perror("read 2 ");
					printf("worker %d:\n", i);
					close(srcfd);
					close(dstfd);
					pthread_exit(0);

				}
				break;
			}
			if ((c = read(dstfd, buf2, c)) <= 0) {
				if (c < 0) {
					perror("read 3");
					printf("worker %d:\n", i);
					close(srcfd);
					close(dstfd);
					pthread_exit(0);
				}
				break;
			}
			if (memcmp(buf1, buf2, c) != 0)
				panic("Worker %d: Buffers not equal", i);
		}
		
		close(srcfd);
		close(dstfd);
#if 0
		if (rename(dstname, "foobar") < 0) {
			perror("rename ");
			printf("worker %d:\n", i);
			pthread_exit(0);
		}
			
		if (unlink("foobar") < 0) {
			perror("unlink ");
			printf("worker %d:\n", i);
			pthread_exit(0);
		}
#else
		if (unlink(dstname) < 0) {
			perror("unlink ");
			printf("worker %d:\n", i);
			pthread_exit(0);
		}
#endif		
	}
	printf("Worker %d done\n", i);
	return 0;
}

/*
 * Remove the existing directory tree and create a new one.
 */
int
dirinit()
{
	int		i;
	char		rootname[256];
	DIR		*dirp;
	struct dirent   *direntp;

	/*
	 * Create the top level directory. If it exists, remove all of
	 * the subdirectories (one level of them) and the files
	 * in each subdir. The following is a simple two-level decend.
	 */
	sprintf(rootname, "%s", TESTDIR);
	if (mkdir(rootname, 0777) < 0) {
		if (errno != EEXIST) {
			perror("cacheinit: mkdir");
			return 1;
		}

		if ((dirp = opendir(rootname)) == 0) {
			perror("cacheinit: opendir");
			return 1;
		}

		/*
		 * Open up each subdir in the cache directory.
		 */
		while ((direntp = readdir(dirp)) != NULL) {
			DIR		*subdirp;
			struct dirent   *subdirentp;
			char		dirname[256];
			char		filename[256];

			/*
			 * Skip . and ..
			 */
			if (strncmp(direntp->d_name, ".",  1) == 0 ||
			    strncmp(direntp->d_name, "..", 2) == 0)
				continue;

			strcpy(dirname, rootname);
			strcat(dirname, "/");
			strcat(dirname, direntp->d_name);
			printf("DIR: %s\n", dirname);

			if ((subdirp = opendir(dirname)) == 0) {
				printf("cacheinit: opensubdir: %s\n", dirname);
				closedir(dirp);
				return 1;
			}

			/*
			 * Go through the subdir and unlink each file.
			 */
			while ((subdirentp = readdir(subdirp)) != NULL) {
				/*
				 * Skip . and ..
				 */
				if (strncmp(subdirentp->d_name, ".", 1) == 0 ||
				    strncmp(subdirentp->d_name, "..", 2) == 0)
					continue;

				strcpy(filename, dirname);
				strcat(filename, "/");
				strcat(filename, subdirentp->d_name);
				printf("FILE: %s\n", filename);

				if (unlink(filename) < 0) {
					printf("cacheinit: unlink %s\n",
					filename);
					closedir(subdirp);
					return 1;
				}

				rewinddir(subdirp);
			}
			closedir(subdirp);

			/*
			 * Now remove the directory.
			 */
			if (rmdir(dirname) < 0) {
				printf("cacheinit: rmdir %s\n", dirname);
				return 1;
			}
			rewinddir(dirp);
		}
		closedir(dirp);
	}

	/*
	 * Now create the cache subdirectories.
	 */
	for (i = 0; i < workers; i++) {
		sprintf(rootname, "%s/%d", TESTDIR, i);

		if (mkdir(rootname, 0777) < 0) {
			perror("cacheinit: mkdir2");
			return 1;
		}
	}
	return 0;
}

void *
checker(void *arg)
{
	while (count) {
		oskit_pthread_sleep(5000);
		if (! activity)
			panic("No activity!");
		activity = 0;
	}
	return 0;
}
