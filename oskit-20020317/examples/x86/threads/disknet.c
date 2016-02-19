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
 * Disk and Net Thrasher: This example is intended to demonstrate a
 * multi-threaded program that uses both the BSD file layer and the BSD
 * socket layer. Its basically a thrasher! A whole bunch of threads are
 * created; 1/2 of them thrash the disk and the other 1/2 thrash the network.
 *
 * Disk Thrasher: Each thread creates a big file of random numbers,
 * and then repeatedly copies it to a destination file, and then
 * rewinds each and compares the two files to make sure it was copied
 * correctly. Each thread writes in different sized chunks. Both files
 * are closed and re-opened each time through the loop.
 *
 * Net Thrasher: Each thread connects to a dopey server running on some
 * regular machine. A big buffer of random numbers is repeatedly sent over,
 * then read back, then compared to make sure it was transferred properly.
 * The server program is contained in dopeyserver.c. See the options below
 * to change the name of the server machine and port number.
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
#include <oskit/startup.h>
#include <oskit/clientos.h>
#include <assert.h>
#include <sys/stat.h>
#include <oskit/queue.h>
#include <time.h>

int		connect_to_host(char *, char *, int *);
int		dirinit();
int		count, activity;
pthread_mutex_t connect_mutex;

#include <oskit/dev/dev.h>
#include <oskit/dev/linux.h>
#ifdef  OSKIT_UNIX
void	start_fs_native_pthreads(char *root);
#endif

#define	MAXTESTERS	20
#define TESTDIR		"/testdir"
#define TESTBYTES	200000
#define DISK_NAME	"wd1"
#define PARTITION_NAME	"b"

int	workers		= MAXTESTERS;
int     testbytes	= TESTBYTES;
char	*server		= "golden-gw";
char	*port		= "6969";

int
main()
{
	pthread_t	foo[1024], bar;
	void		*stat;
	void		*worker(void *arg);
	void		*checker(void *arg);
	int		i;
	char		*option;
	char		*diskname = DISK_NAME;
	char		*partname = PARTITION_NAME;

#ifndef KNIT
	/*
	 * Init the OS.
	 */
	oskit_clientos_init_pthreads();
	
#ifndef OSKIT_UNIX
	start_world_pthreads();
#else
	start_clock();
	start_pthreads();
	start_fs_native_pthreads("/tmp");
	start_network_native_pthreads();
#endif
#endif /* !KNIT */

	if ((option = getenv("WORKERS")) != NULL)
		workers = atoi(option);
	
	if ((option = getenv("TESTSIZE")) != NULL)
		testbytes = atoi(option);
	
	if ((option = getenv("SERVER")) != NULL)
		server = option;
	
	if ((option = getenv("PORT")) != NULL)
		port = option;
	
	printf("Running: %d workers, %d byte files, FS %s%s, "
	       "server %s, port %s\n",
	       workers, testbytes, diskname, partname, server, port);

	pthread_mutex_init(&connect_mutex, 0);
	srand(1);
	dirinit();

	count = workers;
	for (i = 0; i < workers; i++) 
		pthread_create(&foo[i], 0, worker, (void *) i);

	pthread_create(&bar, 0, checker, (void *) 0);

	for (i = 0; i < workers; i++) {
		pthread_join(foo[i], &stat);
		count--;
	}
		
	return 0;
}

void *
worker(void *arg)
{
	int	i = (int) arg;
	void	net_worker(int);
	void	disk_worker(int);

	if (i & 1)
		disk_worker(i);
	else
		net_worker(i);

	return 0;
}

void 
disk_worker(int i)
{
	int	srcfd, dstfd, loop, c, chunksize, count, err;
	char	srcname[128], dstname[128];
	char	*buf1, *buf2;

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
		printf("Disk Worker %d, Loop %d\n", i, loop);
			
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
	}
	printf("Disk Worker %d done\n", i);
}

void
net_worker(int i)
{
	int	sock, loop, c, count;
	char	*buf1, *buf2, *bp;

	printf("Net Worker %d starting\n", i);

	if ((buf1 = malloc(testbytes)) == NULL)
		panic("worker %d: Not enough memory\n", i);

	if ((buf2 = malloc(testbytes)) == NULL)
		panic("worker %d: Not enough memory\n", i);

	for (c = 0; c < testbytes; c++)
		buf1[c] = rand() % 255;

	/*
	 * Connect to the other machine.
	 */
	if (connect_to_host(server, port, &sock)) {
		printf("net_worker: Could not connect!");
		pthread_exit((void *) 1);
	}

	/*
	 * Send a single word telling it how much to expect.
	 */
	count = htonl(testbytes);
	if ((c = send(sock, &count, sizeof(count), 0)) != c) {
		if (c == 0) {
			printf("send 1 EOF:\n");
			goto done;
		}
		else if (c < 0) {
			perror("send 1 syscall");
			goto done;
		}
		else {
			printf("send 1 data\n");
			goto done;
		}
	}
	for (loop = 0; loop < 25; loop++) {
		printf("Net Worker %d, Loop %d\n", i, loop);
		activity = 1;

		/*
		 * Send the source buffer.
		 */
		bp    = buf1;
		count = testbytes;

		while (count) {
			c = (count < 0x4000) ? count : 0x4000;
				
			if ((c = send(sock, bp, c, 0)) <= 0) {
				if (c == 0) {
					printf("send 2 EOF\n");
					goto done;
				}
				else if (c < 0) {
					perror("send 2 syscall");
					goto done;
				}
			}
			count -= c;
			bp    += c;
			sched_yield();
		}

		/*
		 * Read it back.
		 */
		bp    = buf2;
		count = testbytes;

		while (count) {
			c = (count < 0x4000) ? count : 0x4000;

			if ((c = recv(sock, bp, c, 0)) <= 0) {
				if (c == 0) {
					printf("recv EOF\n");
					goto done;
				}
				else if (c < 0) {
					perror("recv syscall");
					goto done;
				}
			}
			count -= c;
			bp    += c;
		}

		/*
		 * Compare.
		 */
		if (memcmp(buf1, buf2, testbytes) != 0)
			panic("Bad compare!");

	}
	
   done:
	printf("Net Worker %d done\n", i);
	shutdown(sock, 2);
	close(sock);
	pthread_exit((void *) 1);
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

int
connect_to_host(char *host, char *port, int *server_sock)
{
	int			sock, data;
	struct sockaddr_in	name;
	struct hostent		*hp, *gethostbyname();
	struct timeval		timeo;

	pthread_mutex_lock(&connect_mutex);

	/* Create socket on which to send. */
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		perror("connect_to_host: opening datagram socket");
		return 1;
	}

	data = 1024 * 32;
	if (setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &data, sizeof(data)) < 0) {
		perror("setsockopt SO_SNDBUF");
		return 1;
	}
	
	data = 1024 * 32;
	if (setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &data, sizeof(data)) < 0) {
		perror("setsockopt SO_RCVBUF");
		return 1;
	}
		
	data = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &data, sizeof(data))
	    < 0) {
		perror("setsockopt SO_KEEPALIVE");
		return 1;
	}

	timeo.tv_sec  = 30;
	timeo.tv_usec = 0;
	if (setsockopt(sock,
		       SOL_SOCKET, SO_RCVTIMEO,
		       &timeo, sizeof(timeo)) < 0) {
		perror("setsockopt SO_RCVTIMEO");
		panic("SETSOCKOPT");
	}

	/*
	 * Construct name, with no wildcards, of the socket to send to.
	 * Gethostbyname() returns a structure including the network address
	 * of the specified host.  The port number is taken from the command
	 * line. 
	 */
	hp = gethostbyname(host);

	if (hp == 0) {
		printf("connect_to_host: unknown host: %s\n", host);
		return 1;
	}
	bcopy(hp->h_addr, &name.sin_addr, hp->h_length);
	name.sin_family = AF_INET;
	name.sin_port = htons((port ? atoi(port) : 80));

	if (connect(sock, (struct sockaddr *) &name, sizeof(name)) < 0) {
		perror("connect_to_host: connecting stream socket");
		return 1;
	}
	pthread_mutex_unlock(&connect_mutex);
	
	*server_sock = sock;
	return 0;
}

#include <syslog.h>
#include <stdarg.h>

void
my_syslog(int pri, const char *fmt, ...)
{
        va_list args;

        va_start(args, fmt);
        osenv_log(pri, fmt, args);
        va_end(args);
}

oskit_syslogger_t oskit_libc_syslogger = my_syslog;

