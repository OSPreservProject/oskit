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
 * Runs some tests on the NetBSD filesystem code and mounts a BMOD FS
 * on it.
 *
 * For this example to work, you need to include at least one bmod
 * in your bootimage, this can be accomplished by
 *
 * 	mkbsdimage this_file some_text_file:whatever
 */

#define DISK_NAME	"sd0"
#define PARTITION_NAME 	"g"
#define BMODDIR 	"/bmod"
#define TESTFILE1	BMODDIR"/test.txt"

#define TEST_RELATIVE_RECURSIVE_SYMLINK		1
#define TEST_ABSOLUTE_RECURSIVE_SYMLINK		1
#define TEST_SIMPLE_READ_WRITE			1
#define TEST_BMOD_MOUNT 			1
#define TEST_DOLS				1
#define TEST_CAT_FIRST_BMOD			1

#include <oskit/dev/dev.h>
#include <oskit/dev/linux.h>

#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>	/* for mkdir ! */
#include <dirent.h>
#include <errno.h>
#include <oskit/fs/memfs.h>
#include <oskit/clientos.h>
#include <oskit/c/fs.h>
#include <assert.h>
#include <oskit/startup.h>

#define CHECK(x)	{ if (-1 == (x)) { perror(#x); \
	printf(__FILE__":%d in "__FUNCTION__"\n", __LINE__); } }

#define CHECK0(x)	{ if (0 == (x)) { perror(#x); \
	printf(__FILE__":%d in "__FUNCTION__"\n", __LINE__); } }

/*
 * open and read a file and write its content to fd #1
 */
void readfile(char *fname)
{
	char	buf[1024];
	int fd, r;

	CHECK(fd = open(fname, O_RDONLY))
	if (fd < 0)
		return;

	printf("---Reading `%s'\n", fname);
	while ((r = read(fd, buf, sizeof buf)) > 0)
		write(1, buf, r);
	if (r < 0)
		perror("read");
	close(fd);
}

void dols(char *, int);

void
main()
{
	oskit_dir_t	*bmodroot;
	char 	*s, fname[256], *dirname = BMODDIR;
	struct dirent *de;
	DIR 	*d;
	int 	fd;
	char	*diskname = DISK_NAME;
	char	*partname = PARTITION_NAME;
	char	*option;

	oskit_clientos_init();
	start_clock();

	if ((option = getenv("DISK")) != NULL)
		diskname = option;

	if ((option = getenv("PART")) != NULL)
		partname = option;

	start_fs(diskname, partname);
#if TEST_BMOD_MOUNT
	bmodroot = start_bmod();
#endif

	printf("---Making dir `%s'\n", dirname);
	CHECK(mkdir(dirname, 0777))

#if TEST_ABSOLUTE_RECURSIVE_SYMLINK
	printf("---Deleting recursive symlink\n");
	CHECK(unlink("/recursive"))

	printf("---Creating recursive symlink\n");
	CHECK(symlink("/recursive", "/recursive"))

	printf("---Trying to access recursive symlink -- should fail\n");
	CHECK(open("/recursive", O_WRONLY | O_CREAT | O_TRUNC, 0755))
#endif

#if TEST_RELATIVE_RECURSIVE_SYMLINK
	printf("---Deleting recursive2 symlink\n");
	CHECK(unlink("/recursive2"))

	printf("---Creating recursive2 symlink\n");
	CHECK(symlink("recursive2", "/recursive2"))

	printf("---Trying to access recursive symlink -- should fail\n");
	CHECK(open("/recursive2", O_WRONLY | O_CREAT | O_TRUNC, 0755))
#endif

#if TEST_SIMPLE_READ_WRITE
	CHECK(fd = open(TESTFILE1, O_WRONLY | O_CREAT | O_TRUNC, 0755))
	s = "This is a short test.\n";
	CHECK(write(fd, s, strlen(s)+1))
	close(fd);

	printf("---Trying to read "TESTFILE1" - should print\n"
		"=====\n%s=====\n", s);
	readfile(TESTFILE1);
#endif

#if TEST_BMOD_MOUNT
	if (-1 == fs_mount(dirname, (oskit_file_t *)bmodroot)) {
		perror("mount");
		exit(-1);
	}
#endif

#if TEST_DOLS
	/*
	 * print a recursive listing of the filesystem, including
	 * the mounted bmod directory
	 */
	printf("\n---Doing a ls -laR /\n");
	dols("/", 1);
#endif

#if TEST_CAT_FIRST_BMOD
	/* search for the first bmod in the bmod directory */
	CHECK0(d = opendir (dirname))

	/* skip . and .. which we know will be returned first */
	for (;;) {
		CHECK0(de = readdir(d))
		if (strcmp(de->d_name, ".") && strcmp(de->d_name, ".."))
			break;
	}
	if (!de)
		exit(0);

	CHECK(chdir(dirname))
	strcpy(fname, de->d_name);
	/* cat it to stdout */
	readfile(fname);
	closedir(d);

	printf("\n---Trying to read "TESTFILE1" again - should fail.\n");
	readfile(TESTFILE1);

	/*
	 * note that the chdir .., followed by the chdir(dirname) is
	 * in fact necessary since the unmount would complete even
	 * though we're holding a reference to the bmod - unmount
	 * doesn't check whether the cwd lies in what's being unmounted
	 * XXX
	 */
	CHECK(chdir(".."))
#endif

#if TEST_BMOD_MOUNT
	printf("\n---Unmounting %s.\n", dirname);
	CHECK(fs_unmount(dirname))
	CHECK(chdir(dirname))

	printf("---Trying to read %s after umount - should fail.\n", fname);
	readfile(fname);
#endif
#if TEST_SIMPLE_READ_WRITE
	printf("---Trying to read "TESTFILE1" again - should work\n");
	readfile(TESTFILE1);
#endif

	exit(0);
}
