/*
 * Copyright (c) 1996, 1998, 1999 University of Utah and the Flux Group.
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
 * Tests the NetBSD filesystem code via the low-level interfaces.
 *
 * You must have a valid ffs filesystem with a dev/sd0g node in it for the
 * test to run.  (You can redefine PARTITION_NAME).
 */


#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <strings.h>
#include <fcntl.h>
#include <stdlib.h>

#include <oskit/dev/dev.h>
#include <oskit/dev/linux.h>
#include <oskit/fs/netbsd.h>
#include <oskit/fs/filesystem.h>
#include <oskit/fs/file.h>
#include <oskit/fs/openfile.h>
#include <oskit/fs/dir.h>
#include <oskit/principal.h>
#include <oskit/boolean.h>
#include <oskit/diskpart/diskpart.h>
#include <oskit/clientos.h>
#include <oskit/startup.h>

#define DISK_NAME	"sd0"
#define PARTITION_NAME	"g"

oskit_principal_t *cur_principal; /* identity of current client process */

oskit_error_t oskit_get_call_context(const struct oskit_guid *iid, void **out_if)
{
    if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||    
	memcmp(iid, &oskit_principal_iid, sizeof(*iid)) == 0) {
	*out_if = cur_principal;
	oskit_principal_addref(cur_principal);
	return 0;
    }
    
    *out_if = 0;
    return OSKIT_E_NOINTERFACE;
}
	

static unsigned int buf[1024];

/* Partition array, filled in by the diskpart library. */
#ifndef OSKIT_UNIX
#define MAX_PARTS 30
static diskpart_t part_array[MAX_PARTS];
#endif

int main(int argc, char **argv)
{
    oskit_blkio_t *disk;
    oskit_blkio_t *part;
    oskit_filesystem_t *fs;
    oskit_dir_t *root, *cwd = 0, *last_cwd;
    oskit_file_t *file;
    oskit_openfile_t *ofile;
    oskit_identity_t id;
    oskit_stat_t sb;
    oskit_u32_t actual, mask;
    oskit_error_t rc;
    char *diskname = DISK_NAME;
    char *partname = PARTITION_NAME;
    char *option;
#ifndef OSKIT_UNIX
    int numparts;
#endif
    int i;

#ifdef OSKIT_UNIX
    if (argc < 2)
	    panic("Usage: %s file\n", argv[0]);
    diskname = argv[1];
    partname = "<bogus>";
#endif

    oskit_clientos_init();
    start_clock();
    start_blk_devices();

    if ((option = getenv("DISK")) != NULL)
	    diskname = option;
	
    if ((option = getenv("PART")) != NULL)
	    partname = option;

    /* initialize the file system code */
    printf(">>>Initializing the file system\n");
    rc = fs_netbsd_init(start_osenv());
    if (rc)
    {
	printf("fs_netbsd_init() failed:  errno 0x%x\n", rc);
	exit(rc);
    }

    /* establish client identity */
    printf(">>>Establishing client identity\n");
    id.uid = 0;
    id.gid = 0;
    id.ngroups = 0;
    id.groups = 0;
    rc = oskit_principal_create(&id, &cur_principal);
    if (rc)
    {
	printf("oskit_principal_create() failed:  errno 0x%x\n", rc);
	exit(rc);
    }

    printf(">>>Opening the disk %s\n", diskname);
    rc = oskit_linux_block_open(diskname,
				OSKIT_DEV_OPEN_READ|OSKIT_DEV_OPEN_WRITE,
				&disk);
    if (rc)
    {
	printf("error 0x%x opening disk%s\n", rc,
	       rc == OSKIT_E_DEV_NOSUCH_DEV ? ": no such device" : "");
	exit(rc);
    }

    printf(">>>Reading partition table and looking for partition %s\n",
	   partname);
#ifdef OSKIT_UNIX
    part = disk;
    oskit_blkio_addref(disk);
#else /* not OSKIT_UNIX */
    numparts = diskpart_blkio_get_partition(disk, part_array, MAX_PARTS);
    if (numparts == 0)
    {
	printf("No partitions found\n");
	exit(1);
    }
    if (diskpart_blkio_lookup_bsd_string(part_array, partname,
					 disk, &part) == 0)
    {
	printf("Couldn't find partition %s\n", partname);
	exit(1);
    }
#endif /* OSKIT_UNIX */

    /* Don't need the disk anymore, the partition has a ref. */
    oskit_blkio_release(disk);

    /* mount a filesystem */    
    printf(">>>Mounting partition %s RDONLY\n", partname);
    rc = fs_netbsd_mount(part, OSKIT_FS_RDONLY, &fs);
    if (rc)
    {
	printf("fs_netbsd_mount() failed:  errno 0x%x\n", rc);
	exit(rc);
    }

    /* Don't need the part anymore, the filesystem has a ref. */
    oskit_blkio_release(part);

    rc = oskit_filesystem_getroot(fs, &root);
    if (rc)
    {
	printf("oskit_filesystem_getroot() failed: errno 0x%x\n", rc);
	exit(rc);
    }

    /* this is a quick, incomplete test of getdirentries */
    {
	    oskit_u32_t	offset = 0;
	    int count;
	    struct oskit_dirents *dirents;
	    oskit_file_t *ff;
	    oskit_error_t rc =
		oskit_dir_getdirentries(root, &offset, 6, &dirents);
	    if (rc) {
		    printf("oskit_dir_getdirentries failed (0x%x)\n", rc);
		    goto bailout;
	    }

	    oskit_dirents_getcount(dirents, &count);
	    printf(">>>First %d entries of /\n", count);
	    for (i = 0; i < count; i++) {
		    char buf[256];
		    struct oskit_dirent *fd = (struct oskit_dirent *) buf;

		    fd->namelen = (256-sizeof(*fd));
		    oskit_dirents_getnext(dirents, fd);
		    printf("   %d %s\n", fd->ino, fd->name);
	    }
	    oskit_dirents_release(dirents);

	    printf(">>>Trying to look up .. in /\n");
	    rc = oskit_dir_lookup(root, "..", &ff);
	    if (rc) {
		    printf("oskit_dir_lookup failed (0x%x)\n", rc);
		    goto bailout;
	    }
	    oskit_file_release(ff);
    }

    /* remount the file system read-write */
    rc = oskit_filesystem_remount(fs, 0);
    if (rc)
    {
	printf("oskit_filesystem_remount() failed:  errno 0x%x\n", rc);
	exit(rc);
    }

    /* create a directory */
    printf(">>>Creating /a\n");
    rc = oskit_dir_mkdir(root, "a", 0755);
    if (rc)
    {
	printf("mkdir(/a) failed:  errno 0x%x\n", rc);
	exit(rc);
    }

    printf(">>>Obtaining a handle to /a\n");
    rc = oskit_dir_lookup(root, "a", (oskit_file_t **)&cwd);
    if (rc)
    {
	printf("lookup(/a) failed:  errno 0x%x\n", rc);
	exit(rc);
    }    

    /* create a subdirectory */
    printf(">>>Creating /a/b\n");
    rc = oskit_dir_mkdir(cwd, "b", 0700);
    if (rc)
    {
	printf("mkdir(/a/b) failed:  errno 0x%x\n", rc);
	exit(rc);
    }

    printf(">>>Obtaining a handle to /a/b\n");
    last_cwd = cwd;
    rc = oskit_dir_lookup(cwd, "b", (oskit_file_t **)&cwd);
    if (rc)
    {
	printf("lookup(/a/b) failed:  errno 0x%x\n", rc);
	exit(rc);
    }

    /* create a file in the subdirectory */
    printf(">>>Creating foo\n");
    rc = oskit_dir_create(cwd, "foo", 0, 0600, &file);
    if (rc)
    {
	printf("create(foo) failed:  errno 0x%x\n", rc);
	exit(rc);
    }
    
    /* open the file */
    printf(">>>Opening foo\n");
    rc = oskit_file_open(file, OSKIT_O_RDWR, &ofile);
    if (rc)
    {
	printf("open(foo) failed:  errno 0x%x\n", rc);
	exit(rc);
    }    

    /* fill the buffer with a nonrepeating integer sequence */
    for (i = 0; i < sizeof(buf)/sizeof(unsigned int); i++)
	    buf[i] = i; 

    /* write the sequence */
    printf(">>>Writing to foo\n");
    rc = oskit_openfile_write(ofile, buf, sizeof(buf), &actual);
    if (rc)
    {
	printf("write failed:  errno 0x%x\n", rc);
	exit(rc);
    }

    if (actual != sizeof(buf))
    {
	printf("only wrote %d bytes of %d total\n",
	       actual, sizeof(buf));
	exit(1);
    }

    /* reopen the file */
    printf(">>>Reopening foo\n");
    oskit_openfile_release(ofile);
    rc = oskit_file_open(file, OSKIT_O_RDWR, &ofile);
    if (rc)
    {
	printf("open(foo) failed:  errno 0x%x\n", rc);
	exit(rc);
    }    

    bzero(buf, sizeof(buf));

    /* read the file contents */
    printf(">>>Reading foo\n");
    rc = oskit_openfile_read(ofile, buf, sizeof(buf), &actual);
    if (rc)
    {
	printf("read failed:  errno 0x%x\n", rc);
	exit(rc);
    }

    if (actual != sizeof(buf))
    {
	printf("only read %d bytes of %d total\n", actual, sizeof(buf));
	exit(1);
    }

    oskit_openfile_release(ofile);
    
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
    rc = oskit_dir_link(cwd, "bar", file);
    if (rc)
    {
	printf("link(foo, bar) failed:  errno 0x%x\n", rc);	    
	exit(rc);
    }

    printf(">>>Renaming bar to rab\n");
    rc = oskit_dir_rename(cwd, "bar", cwd, "rab");
    if (rc)
    {
	printf("rename(bar, rab) failed:  errno 0x%x\n", rc);	    
	exit(rc);
    }

    printf(">>>Stat'ing foo\n");
    bzero(&sb, sizeof(sb));
    rc = oskit_file_stat(file, &sb);
    if (rc)
    {
	printf("stat(foo) failed:  errno 0x%x\n", rc);	    
	exit(rc);
    }

    printf(">>>Checking foo's mode and link count\n");
    if (!OSKIT_S_ISREG(sb.mode) || ((sb.mode & 0777) != 0600))
    {
	printf("foo has the wrong mode (0x%x)\n", sb.mode);
	exit(1);
    }

    if ((sb.nlink != 2))
    {
	printf("foo does not have 2 links?, nlink=%d\n", sb.nlink);
	exit(1);
    }

    printf(">>>Stat'ing .\n");
    bzero(&sb, sizeof(sb));
    rc = oskit_dir_stat(cwd, &sb);
    if (rc)
    {
	printf("stat(a/b) failed:  errno 0x%x\n", rc);	    
	exit(rc);
    }

    printf(">>>Checking .'s mode and link count\n");
    if (!OSKIT_S_ISDIR(sb.mode) || ((sb.mode & 0777) != 0700))
    {
	printf("a/b has the wrong mode (0x%x)\n", sb.mode);
	exit(1);
    }

    if ((sb.nlink != 2))
    {
	printf("a/b does not have 2 links?, nlink=%d\n", sb.nlink);
	exit(1);
    }

    printf(">>>Stat'ing ..\n");
    bzero(&sb, sizeof(sb));
    rc = oskit_dir_stat(last_cwd, &sb);
    if (rc)
    {
	printf("stat(a) failed:  errno 0x%x\n", rc);	    
	exit(rc);
    }

    printf(">>>Checking ..'s mode and link count\n");
    if (!OSKIT_S_ISDIR(sb.mode) || ((sb.mode & 0777) != 0755))
    {
	printf("a has the wrong mode (0x%x)\n", sb.mode);
	exit(1);
    }

    if ((sb.nlink != 3))
    {
	printf("a does not have 3 links?, nlink=%d\n", sb.nlink);
	exit(1);
    }

    printf(">>>Changing foo's mode\n");
    mask = OSKIT_STAT_MODE;
    sb.mode = 0666;
    rc = oskit_file_setstat(file, mask, &sb);
    if (rc)
    {
	printf("chmod(foo) failed:  errno 0x%x\n", rc);	        
	exit(rc);
    }

    printf(">>>Changing foo's user and group\n");
    mask = OSKIT_STAT_UID | OSKIT_STAT_GID; 
    sb.uid = 5458;
    sb.gid = 101;
    rc = oskit_file_setstat(file, mask, &sb);
    if (rc)
    {
	printf("chown(foo) failed:  errno 0x%x\n", rc);	        
	exit(rc);
    }

    printf(">>>Stat'ing foo\n");
    bzero(&sb, sizeof(sb));
    rc = oskit_file_stat(file, &sb);
    if (rc)
    {
	printf("stat(foo) failed:  errno 0x%x\n", rc);	    
	exit(rc);
    }

    printf(">>>Checking foo's mode and ownership\n");
    if (!OSKIT_S_ISREG(sb.mode) || ((sb.mode & 0777) != 0666))
    {
	printf("chmod(foo) didn't work properly\n");
	exit(1);
    }

    if (sb.uid != 5458)
    {
	printf("chown(foo) didn't work properly\n");
	exit(1);
    }

    if (sb.gid != 101)
    {
	printf("chown(foo) didn't work properly\n");
	exit(1);
    }

    printf(">>>Unlinking foo\n");
    oskit_file_release(file);
    rc = oskit_dir_unlink(cwd, "foo");
    if (rc)
    {
	printf("unlink(foo) didn't work properly\n");
	exit(rc);
    }

    printf(">>>Unlinking rab\n");
    rc = oskit_dir_unlink(cwd, "rab");
    if (rc)
    {
	printf("unlink(rab) didn't work properly\n");
	exit(rc);
    }

    printf(">>>Changing cwd to ..\n");
    oskit_dir_release(cwd);
    cwd = last_cwd;

    printf(">>>Removing b\n");
    rc = oskit_dir_rmdir(cwd, "b");
    if (rc)
    {
	printf("rmdir(b) didn't work properly\n");
	exit(rc);
    }

    printf(">>>Changing cwd to ..\n");
    oskit_dir_release(cwd);
    cwd = root;

    printf(">>>Removing a\n");
    rc = oskit_dir_rmdir(cwd, "a");
    if (rc)
    {
	printf("rmdir(a) didn't work properly\n");
	exit(rc);
    }

bailout:

    printf(">>>Syncing\n");
    if (cwd) oskit_dir_release(cwd);
    oskit_filesystem_sync(fs,TRUE);	

    printf(">>>Unmounting\n");
    oskit_filesystem_release(fs);

    printf(">>>Exiting\n");
    exit(0);
}

