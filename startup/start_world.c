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
 * Convenience function to start up clock, disk filesystem, swap, and network.
 */

#include <oskit/startup.h>
#include <oskit/c/stdio.h>
#include <oskit/c/stdlib.h>
#include <oskit/c/string.h>
#include <oskit/c/unistd.h>
#include <oskit/c/sys/types.h>
#include <oskit/c/sys/stat.h>
#include <oskit/c/ctype.h>
#include <oskit/io/absio.h>
#include <oskit/io/blkio.h>
#include <oskit/c/fs.h>
#include <oskit/fs/memfs.h>
#include <oskit/svm.h>
#include <oskit/clientos.h>
#include <oskit/com/libcenv.h>


#ifdef PTHREADS
#include <oskit/threads/pthread.h>
#define	start_world		start_world_pthreads
#define	start_console		start_console_pthreads
#define	start_network		start_network_pthreads
#define	start_fs		start_fs_pthreads
#define	start_fs_on_blkio	start_fs_on_blkio_pthreads
#define	start_fs_bmod		start_fs_bmod_pthreads
#define	start_svm		start_svm_pthreads
#define	start_svm_on_blkio	start_svm_on_blkio_pthreads
#endif


static int disk_option(const char *option, const char *ro_option,
		       oskit_blkio_t **out_bio);

void
start_world(void)
{
	oskit_blkio_t *bio;
	oskit_error_t rc;
	static int run;

	if (run)
		return;
	run = 1;

	/*
	 * Initialize the startup library atexit code.
	 */
	startup_atexit_init();

	/*
	 * Start the system clock.
	 */
	start_clock();

#ifdef PTHREADS
	start_pthreads();
#endif

	/*
	 * Initialize and probe device drivers.
	 */
	osenv_process_lock();
	start_devices();
	osenv_process_unlock();


	/*
	 * If we have a "root=device" argument, start the disk filesystem.
	 */
	if (disk_option("root", "read-only", &bio)) {
		start_fs_on_blkio(bio);

		if (getenv("no-bmod"))
			goto skipbmod;
		
		/*
		 * Mount the bmod also, in a well-known place.
		 */
		if (mkdir("/bmod", 0) < 0 && errno != EEXIST) {
			perror("start_world: Could not mkdir /bmod\n");
			goto skipbmod;
		}

		if ((rc = fs_mount("/bmod",
				   (oskit_file_t *) start_bmod())) != 0)
			printf("start_world: fs_mount failed\n");
	skipbmod:
	}
	else {
		/*
		 * Otherwise just set up the bmod filesystem.
		 */
		start_fs_bmod();

		/*
		 * Create its well-known name.
		 */
		if (symlink("/", "/bmod") < 0)
			perror("start_world: Could not symlink /bmod --> /\n");
	}

	/*
	 * If we have a "swap=device" argument, start paging to that disk.
	 */
	if (disk_option("swap", 0, &bio)) {
		start_svm_on_blkio((oskit_blkio_t *) bio);
		
		/*
		 * Don't need the partition anymore, SVM has a ref.
		 */
		oskit_blkio_release(bio);
	}
	else if (!getenv("no-svm")) {
		/*
		 * Otherwise, no paging.
		 */
		start_svm((char *) NULL, (char *) NULL);
	}

	/*
	 * Start the network.
	 */
	if (!getenv("no-network"))
		start_network();

#ifdef  GPROF
	/*
	 * Fire up the gprof support.
	 */
	start_gprof();
#endif
}

/*
 * Check for an environment variable named by `option' whose value
 * is a disk device/partition name in some vaguely canonical format.
 * Return nonzero iff *out_bio is the successfully opened disk partition.
 * This tries to grok either BSD and Linux device and (optional)
 * partition syntax, or any combination of flavors, and strips
 * a leading "/dev/" which people like to prepend to the device name.
 */
static int
disk_option(const char *option, const char *ro_option,
	    oskit_blkio_t **out_bio)
{
	int rc;
	char *s, *disk, *part;

	s = getenv(option);
	if (!s)
		return 0;

	if (!strncmp(s, "/dev/", 5))
		s += 5;
	disk = strdup(s);
	if (disk[0] == 'w' && disk[1] == 'd') /* BSD wd => Linux hd */
		disk[0] = 'h';
	if (disk[1] == 'd' && (disk[0] == 's' || disk[0] == 'h')) {
		if (isdigit(disk[2])) /* linux uses hda, not hd0 */
			disk[2] = disk[2] - '0' + 'a';
		if (disk[3] == '\0')
			part = 0; /* use whole disk */
		else if (isdigit(disk[3])) {
				/* insert "s" for slice syntax */
			part = strdup(&disk[2]);
			part[0] = 's';
			disk[3] = '\0';	/* terminate disk */
		}
		else {
			part = strdup(&disk[3]);
			disk[3] = '\0';
		}
	}
	else {
		/*
		 * Can't guess syntax, presume it was for whole disk.
		 */
		disk = s;
		part = 0;
	}

	osenv_process_lock();
	rc = start_disk(disk, part, ro_option && getenv(ro_option),
			out_bio);
	osenv_process_unlock();
	free(disk);
	if (part)
		free(part);
	if (rc) {
		printf("start_disk() failed:  errno 0x%x\n", rc);
		exit(rc);
	}

	return 1;
}
