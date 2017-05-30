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
 * This demonstrates the oskit filesystem COM interfaces going to the
 * Linux filesytems.
 */

#if 0
#define DISK_NAME	"wd1"		/* pencil */
#define PARTITION_NAME	"e"
#else
#define DISK_NAME	"sd0"		/* shaky */
#define PARTITION_NAME	"g"
#endif

#include <oskit/dev/dev.h>
#include <oskit/dev/linux.h>
#include <oskit/io/blkio.h>
#include <oskit/fs/filesystem.h>
#include <oskit/fs/dir.h>
#include <oskit/fs/openfile.h>
#include <oskit/fs/linux.h>
#include <oskit/diskpart/diskpart.h>
#include <oskit/principal.h>
#include <oskit/clientos.h>
#include <oskit/startup.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

/* Partition array, filled in by the diskpart library. */
#ifndef OSKIT_UNIX
#define MAX_PARTS 30
static diskpart_t part_array[MAX_PARTS];
#endif

/* Identity of current client process. */
static oskit_principal_t *cur_principal;

#define CHECK(err, f, args) ({			\
	(err) = f args;			\
	if (err)				\
		panic(#f" failed 0x%lx", (err));\
})
#define HMM(err, f, args) ({			\
	(err) = f args;			\
	if (err)				\
		printf("** " #f " failed 0x%x\n", (err));\
})

oskit_error_t
oskit_get_call_context(const struct oskit_guid *iid, void **out_if)
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

static void
showstat(oskit_stat_t *sb)
{
	printf("   dev    : 0x%x\n", sb->dev);
	printf("   ino	  : %d\n", sb->ino);
	printf("   mode	  : 0%o\n", sb->mode);
	printf("   nlink  : %d\n", sb->nlink);
	printf("   uid	  : %d\n", sb->uid);
	printf("   gid	  : %d\n", sb->gid);
	printf("   rdev	  : %d\n", sb->rdev);
	printf("   atime  : (%d,%d)\n", sb->atime.tv_sec, sb->atime.tv_nsec);
	printf("   mtime  : (%d,%d)\n", sb->mtime.tv_sec, sb->mtime.tv_nsec);
	printf("   ctime  : (%d,%d)\n", sb->ctime.tv_sec, sb->ctime.tv_nsec);
	printf("   size	  : 0x%08lx%08lx\n",
	       (unsigned long)(sb->size>>32),
	       (unsigned long)(sb->size&0xffffffff));
	printf("   blocks : 0x%08lx%08lx\n",
	       (unsigned long)(sb->blocks>>32),
	       (unsigned long)(sb->blocks&0xffffffff));
	printf("   blksize: %d\n", sb->blksize);
}

int
main(int argc, char **argv)
{
	char buf[BUFSIZ];
	oskit_blkio_t *disk;
	oskit_blkio_t *part;
	oskit_error_t err;
	oskit_identity_t id;
	oskit_filesystem_t *rootfs;
	oskit_dir_t *dir;
	oskit_file_t *file, *file2;
	oskit_stat_t sb;
	oskit_statfs_t sfs;
#ifndef OSKIT_UNIX
	int numparts;
#endif
	oskit_u32_t actual;
	char *diskname = DISK_NAME;
	char *partname = PARTITION_NAME;
	char *option;
	oskit_mode_t savedmode;

#ifdef OSKIT_UNIX
	if (argc < 2)
		panic("Usage: %s file\n", argv[0]);
	diskname = argv[1];
	partname = "<bogus>";
#endif

	if ((option = getenv("DISK")) != NULL)
		diskname = option;

	if ((option = getenv("PART")) != NULL)
		partname = option;

	oskit_clientos_init();
	start_clock();				/* so gettimeofday works */
	start_blk_devices();

	printf(">>>Initializing filesystem...\n");
	CHECK(err, fs_linux_init, ());

	printf(">>>Establishing client identity\n");
	id.uid = 0;
	id.gid = 0;
	id.ngroups = 0;
	id.groups = 0;
	CHECK(err, oskit_principal_create, (&id, &cur_principal));

	printf(">>>Opening the disk %s\n", diskname);
	err = oskit_linux_block_open(diskname,
				     OSKIT_DEV_OPEN_READ|OSKIT_DEV_OPEN_WRITE,
				     &disk);
	if (err)
		panic("error 0x%x opening disk%s", err,
		      err == OSKIT_E_DEV_NOSUCH_DEV ? ": no such device" : "");

	printf(">>>Reading partition table and looking for partition %s\n",
	       partname);
#ifdef OSKIT_UNIX
	part = disk;
	oskit_blkio_addref(disk);
#else /* not OSKIT_UNIX */
	numparts = diskpart_blkio_get_partition(disk, part_array, MAX_PARTS);
	if (numparts == 0)
		panic("No partitions found");
	if (diskpart_blkio_lookup_bsd_string(part_array, partname,
					     disk, &part) == 0)
		panic("Couldn't find partition %s", partname);
#endif /* OSKIT_UNIX */

	/* Don't need the disk anymore, the partition has a ref. */
	oskit_blkio_release(disk);

	printf(">>>Mounting partition %s RDONLY\n", partname);
	CHECK(err, fs_linux_mount, (part, OSKIT_FS_RDONLY, &rootfs));

	/* Don't need the part anymore, the filesystem has a ref. */
	oskit_blkio_release(part);

	printf(">>>Doing statfs on partition %s\n", partname);
	CHECK(err, oskit_filesystem_statfs, (rootfs, &sfs));
	printf("   file system block size                : %d\n", sfs.bsize);
	printf("   fundamental file system block size    : %d\n", sfs.frsize);
	printf("   total blocks in fs in units of frsize : %d\n", sfs.blocks);
	printf("   free blocks in fs                     : %d\n", sfs.bfree);
	printf("   free blocks avail to non-superuser    : %d\n", sfs.bavail);
	printf("   total file nodes in file system       : %d\n", sfs.files);
	printf("   free file nodes in fs                 : %d\n", sfs.ffree);
	printf("   free file nodes avail to non-superuser: %d\n", sfs.favail);
	printf("   file system id                        : %d\n", sfs.fsid);
	printf("   mount flags                           : %d\n", sfs.flag);
	printf("   max bytes in a file name              : %d\n", sfs.namemax);

	printf(">>>Sync'ing filesystem\n");
	HMM(err, oskit_filesystem_sync, (rootfs, 1));

	printf(">>>Remounting R/W\n");
	CHECK(err, oskit_filesystem_remount, (rootfs, 0));

	printf(">>>Getting a handle to the root dir\n");
	CHECK(err, oskit_filesystem_getroot, (rootfs, &dir));

	printf(">>>stat'ing rootdir\n");
	CHECK(err, oskit_dir_stat, (dir, &sb));
	showstat(&sb);

	printf(">>>Changing owner to owner++\n");
	sb.uid++;
	HMM(err, oskit_dir_setstat, (dir, OSKIT_STAT_UID, &sb));

	printf(">>>Syncing root dir\n");
	HMM(err, oskit_dir_sync, (dir, 1));

	printf(">>>Testing rootdir for executability\n");
	HMM(err, oskit_dir_access, (dir, OSKIT_X_OK));

	printf(">>>Changing mode to 0444\n");
	savedmode = sb.mode;
	sb.mode = 0444;
	HMM(err, oskit_dir_setstat, (dir, OSKIT_STAT_MODE, &sb));

	printf(">>>Testing rootdir for executability as uid 1084 (should fail)\n");
	id.uid = 1084;
	CHECK(err, oskit_principal_create, (&id, &cur_principal));
	HMM(err, oskit_dir_access, (dir, OSKIT_X_OK));
	id.uid = 0;
	CHECK(err, oskit_principal_create, (&id, &cur_principal));

	printf(">>>Changing mode back to 0%o\n", savedmode);
	sb.mode = savedmode;
	HMM(err, oskit_dir_setstat, (dir, OSKIT_STAT_MODE, &sb));

	printf(">>>Changing owner back to %d\n", sb.uid-1);
	sb.uid--;
	HMM(err, oskit_dir_setstat, (dir, OSKIT_STAT_UID, &sb));

	printf(">>>Creating link -> /hi/my/name/is/billy\n");
	err = oskit_dir_symlink(dir, "link", "/hi/my/name/is/billy");
	if (err)
		printf("** symlink failed 0x%x\n", err);
	else {
		printf(">>>Looking up `link'\n");
		CHECK(err, oskit_dir_lookup, (dir, "link", &file));
		printf(">>>Readlink on `link'\n");
		CHECK(err, oskit_file_readlink, (file, buf, BUFSIZ, &actual));
		buf[actual] = '\0';
		printf("   link -> %s\n", buf);
		printf(">>>Creating `hardlink' to `link'\n");
		CHECK(err, oskit_dir_link, (dir, "hardlink", file));
		printf(">>>Releasing ref to `link'\n");
		assert(oskit_file_release(file) == 0);
		printf(">>>Removing `link'\n");
		CHECK(err, oskit_dir_unlink, (dir, "link"));
		printf(">>>Removing `hardlink'\n");
		CHECK(err, oskit_dir_unlink, (dir, "hardlink"));
	}

	printf(">>>Making directory `dir'\n");
	HMM(err, oskit_dir_mkdir, (dir, "dir", 0777));
	printf(">>>Removing directory `dir'\n");
	HMM(err, oskit_dir_rmdir, (dir, "dir"));

	printf(">>>Making special file `special', mode 020777, (11,22)\n");
	err = oskit_dir_mknod(dir, "special", 020777, (11<<8)|(22));
	if (err)
		printf("** mknod failed 0x%x\n", err);
	else {
		printf(">>>Looking up special\n");
		CHECK(err, oskit_dir_lookup, (dir, "special", &file));
		printf(">>>stat'ing `special'\n");
		CHECK(err, oskit_file_stat, (file, &sb));
		showstat(&sb);
		printf(">>>Releasing ref to `special'\n");
		assert(oskit_file_release(file) == 0);
		printf(">>>Removing `special'\n");
		CHECK(err, oskit_dir_unlink, (dir, "special"));
	}

	printf(">>>Looking up .. in rootdir\n");
	CHECK(err, oskit_dir_lookup, (dir, "..", &file));
	oskit_file_release(file);
	printf(">>>Looking up lost+found in rootdir\n");
	err = oskit_dir_lookup(dir, "lost+found", &file);
	if (err)
		printf("** lookup of lost+found failed 0x%x\n", err);
	else {
		printf(">>>Looking up .. in lost+found\n");
		CHECK(err, oskit_dir_lookup, ((oskit_dir_t *)file, "..", &file2));
		oskit_file_release(file2);
		assert(oskit_file_release(file) == 0);
	}

	printf(">>>Creating file `hi'\n");
	err = oskit_dir_create(dir, "hi", 0, 0777, &file);
	if (err) {
		printf("** create of \"hi\" failed 0x%x\n", err);
		goto nocreate;
	}
	printf(">>>stat'ing `hi'\n");
	CHECK(err, oskit_file_stat, (file, &sb));
	showstat(&sb);
	assert(oskit_file_release(file) == 0);
	printf(">>>Moving `hi' to `lost+found/bye'\n");
	err = oskit_dir_lookup(dir, "lost+found", &file);
	if (err) {
		printf("** lookup of lost+found failed 0x%x\n", err);
		CHECK(err, oskit_dir_unlink, (dir, "hi"));
	}
	else {
		CHECK(err, oskit_dir_rename, (dir, "hi", (oskit_dir_t *)file, "bye"));
		printf(">>>Removing `lost+found/bye'\n");
		CHECK(err, oskit_dir_unlink, ((oskit_dir_t *)file, "bye"));
		assert(oskit_dir_release((oskit_dir_t *)file) == 0);
	}
 nocreate:

	printf(">>>getdirentries\n");
	{
		oskit_u32_t offset = 0;
		int i, count;
		struct oskit_dirents *de;
		char buf[256];
		struct oskit_dirent *fd = (struct oskit_dirent *) buf;

		CHECK(err, oskit_dir_getdirentries, (dir, &offset, 10, &de));
		CHECK(err, oskit_dirents_getcount,  (de, &count));

		for (i = 0; i < count; i++) {
			fd->namelen = (256-sizeof(*fd));
			CHECK(err, oskit_dirents_getnext, (de, fd));

			printf("   %d %s\n", fd->ino, fd->name);
		}
		assert(oskit_dirents_release(de) == 0);

		CHECK(err, oskit_dir_getdirentries, (dir, &offset, 2, &de));

		/* Could hit EOF */
		if (de != NULL) {
			CHECK(err, oskit_dirents_getcount,  (de, &count));

			for (i = 0; i < count; i++) {
				fd->namelen = (256-sizeof(*fd));
				CHECK(err, oskit_dirents_getnext, (de, fd));

				printf("   %d %s\n", fd->ino, fd->name);
			}
			assert(oskit_dirents_release(de) == 0);
		}
	}

	printf(">>>Open\n");
	do {
		oskit_openfile_t *of;
		int i;
		oskit_u64_t off;

		err = oskit_dir_create(dir, "foo", 0, 0777, &file);
		if (err)
			break;
/*
 * Either should do the job.
 * One tests the absio interface, the other the stream interface.
 */
#if 1
		err = oskit_file_open(file, OSKIT_O_RDWR, &of);
#else
		err = oskit_soa_openfile_from_file(file, OSKIT_O_RDWR, &of);
#endif
		if (err) {
			assert(oskit_file_release(file) == 0);
			break;
		}
		for (i = 0; i < BUFSIZ; i++)
			buf[i] = (char)i;
		CHECK(err, oskit_openfile_write, (of, buf, BUFSIZ, &actual));
		CHECK(err, oskit_openfile_write, (of, buf, BUFSIZ, &actual));
		CHECK(err, oskit_openfile_seek, (of, BUFSIZ/2, OSKIT_SEEK_SET, &off));
		CHECK(err, oskit_openfile_read, (of, buf, BUFSIZ, &actual));
		for (i = 0; i < BUFSIZ; i++)
			if (buf[i] != (char)(i+BUFSIZ/2))
				printf("wrong at char %d\n", i);
		assert(oskit_openfile_release(of) == 0);
		assert(oskit_file_release(file) == 0);
		CHECK(err, oskit_dir_unlink, (dir, "foo"));
	} while (0);

	printf(">>>Releasing rootdir\n");
	assert(oskit_dir_release(dir) == 0);
	printf(">>>Unmounting %s\n", partname);
	assert(oskit_filesystem_release(rootfs) == 0);

	return 0;
}
