/*
 * Copyright (c) 1996-2000 University of Utah and the Flux Group.
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
 * Tests the NetBSD filesystem code via the POSIX interfaces.
 *
 * This does the same things as ../netbsd_fs_com.c.
 *
 * Compare also unsupported/start_fs.c
 *
 * You must have a valid filesystem with a dev/sd0g node in it for the
 * test to run.  (You can redefine PARTITION_NAME).
 */
#define DISK_NAME	"sd0"
#define PARTITION_NAME	"g"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>

#include <oskit/clientos.h>
#include <oskit/startup.h>
#include <oskit/boolean.h>
#include <oskit/c/fs.h>

static unsigned int buf[1024];

int main(int argc, char **argv)
{
    struct stat sb;
    oskit_u32_t actual;
    char *diskname = DISK_NAME;
    char *partname = PARTITION_NAME;
    char *option;
    int 	fd, i;

#define CHECK(x)        { if (-1 == (x)) { perror(#x); \
	printf(__FILE__":%d in "__FUNCTION__"\n", __LINE__); exit(-1); } }

#ifndef KNIT
    if ((option = getenv("DISK")) != NULL)
	    diskname = option;
	
    if ((option = getenv("PART")) != NULL)
	    partname = option;

    /*
     * Create the Client OS. For this example, use the startup library
     * to start the disk and get it mounted. There are plenty of other
     * example programs with all the goo spelled out in detail.
     */
    oskit_clientos_init();
#ifndef OSKIT_UNIX
    start_fs(diskname, partname);
#else
    start_fs("/dev/oskitfs", 0);
#endif
#endif
    
    /* create a directory */
    printf(">>>Creating /foo\n");
    CHECK(mkdir("/foo", 0755))

    printf(">>>Changing into /foo\n");
    CHECK(chdir("/foo"))

    /* create a subdirectory */
    printf(">>>Creating /foo/b\n");
    CHECK(mkdir("b", 0700))

    printf(">>>Changing into /foo/b\n");
    CHECK(chdir("b"))

    /* create a file in the subdirectory */
    printf(">>>Creating foo\n");
    CHECK(fd = open("foo", O_CREAT | O_RDWR, 0600)) 

    /* fill the buffer with a nonrepeating integer sequence */
    for (i = 0; i < sizeof(buf)/sizeof(unsigned int); i++)
	    buf[i] = i; 

    /* write the sequence */
    printf(">>>Writing to foo\n");
    CHECK(actual = write(fd, buf, sizeof buf))

    if (actual != sizeof(buf))
    {
	printf("only wrote %d bytes of %d total\n",
	       actual, sizeof(buf));
	exit(1);
    }

    /* reopen the file */
    printf(">>>Reopening foo\n");
    CHECK(close(fd))

    CHECK(fd = open("foo", O_RDWR)) 
    memset(buf, 0, sizeof(buf));

    /* read the file contents */
    printf(">>>Reading foo\n");
    CHECK(actual = read(fd, buf, sizeof buf))

    if (actual != sizeof(buf))
    {
	printf("only read %d bytes of %d total\n", actual, sizeof(buf));
	exit(1);
    }

    CHECK(close(fd))
    
    /* check the file contents */
    printf(">>>Checking foo's contents\n");
    for (i = 0; i < sizeof(buf)/sizeof(unsigned int); i++)
    {
	if (buf[i] != i)
	{
	    printf("word %d of file was corrupted\n", i);
	    exit(1);
	}
    }

    /* lots of other tests */
    printf(">>>Linking bar to foo\n");
    CHECK(link("foo", "bar"))

    printf(">>>Renaming bar to rab\n");
    CHECK(rename("bar", "rab"))

    printf(">>>Stat'ing foo\n");
    memset(&sb, 0, sizeof(sb));
    CHECK(stat("foo", &sb))

    printf(">>>Checking foo's mode and link count\n");
    if (!S_ISREG(sb.st_mode) || ((sb.st_mode & 0777) != 0600))
    {
	printf("foo has the wrong mode (0%o)\n", sb.st_mode);
	exit(1);
    }

    if ((sb.st_nlink != 2))
    {
	printf("foo does not have 2 links?, nlink=%d\n", sb.st_nlink);
	exit(1);
    }

    printf(">>>Stat'ing .\n");
    memset(&sb, 0, sizeof(sb));
    CHECK(stat(".", &sb))

    printf(">>>Checking .'s mode and link count\n");
    if (!S_ISDIR(sb.st_mode) || ((sb.st_mode & 0777) != 0700))
    {
	printf("/foo/b has the wrong mode (0%o)\n", sb.st_mode);
	exit(1);
    }

    if ((sb.st_nlink != 2))
    {
	printf("/foo/b does not have 2 links?, nlink=%d\n", sb.st_nlink);
	exit(1);
    }

    printf(">>>Stat'ing ..\n");
    memset(&sb, 0, sizeof(sb));
    CHECK(stat("..", &sb))

    printf(">>>Checking ..'s mode and link count\n");
    if (!S_ISDIR(sb.st_mode) || ((sb.st_mode & 0777) != 0755))
    {
	printf("foo has the wrong mode (0%o)\n", sb.st_mode);
	exit(1);
    }

    if ((sb.st_nlink != 3))
    {
	printf("foo does not have 3 links?, nlink=%d\n", sb.st_nlink);
	exit(1);
    }

    printf(">>>Changing foo's mode\n");
    CHECK(chmod("foo", 0666))

    printf(">>>Changing foo's user and group\n");
    CHECK(chown("foo", 5458, 101))

    printf(">>>Stat'ing foo\n");
    memset(&sb, 0, sizeof(sb));
    CHECK(stat("foo", &sb))

    printf(">>>Checking foo's mode and ownership\n");
    if (!S_ISREG(sb.st_mode) || ((sb.st_mode & 0777) != 0666))
    {
	printf("chmod(foo) didn't work properly\n");
	exit(1);
    }

    if (sb.st_uid != 5458)
    {
	printf("chown(foo) didn't work properly\n");
	exit(1);
    }

    if (sb.st_gid != 101)
    {
	printf("chown(foo) didn't work properly\n");
	exit(1);
    }

    printf(">>>Unlinking foo\n");
    CHECK(unlink("foo"))

    printf(">>>Unlinking rab\n");
    CHECK(unlink("rab"))

    printf(">>>Changing cwd to ..\n");
    CHECK(chdir(".."))

    printf(">>>Removing b\n");
    CHECK(rmdir("b"))

    printf(">>>Changing cwd to ..\n");
    CHECK(chdir(".."))

    printf(">>>Removing foo\n");
    CHECK(rmdir("foo"))

    printf(">>>Exiting\n");
    exit(0);
}

