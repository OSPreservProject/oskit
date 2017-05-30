/*
 * Copyright (c) 1999, 2000 University of Utah and the Flux Group.
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
 * populate the bmod file system with a sub tree of the real file system.
 * The best way to use this code is by calling start_fs_native_bmod() in
 * the startup library. It will create a memory filesystem, and then pass
 * the root directory of that filesystem to:
 *
 *	int
 *	oskit_unix_bmod_copydir(oskit_dir_t *rootdir, const char *path)
 *
 * will create all the directories in the path, and then copy the directory
 * tree at the last component into the bmod fs at the equiv location.
 *
 * Of course, writes will end up in the bmodfs and hence be transient.
 */
#include <oskit/fs/file.h>
#include <oskit/fs/openfile.h>
#include <oskit/fs/dir.h>
#include <oskit/fs/memfs.h>
#include <oskit/fs/soa.h>
#include <oskit/startup.h>

#include "native.h"

static int		verbose;
static void		copy_dir(int, oskit_dir_t *);
static oskit_dir_t     *make_dir(oskit_dir_t *dir, const char *path);

int
oskit_unix_bmod_copydir(oskit_dir_t *rootdir, const char *path)
{
	oskit_dir_t	*dir;
	int		fd;

	printf("unix/bmodfs: Creating %s\n", path);

	if (path[0] == '.' && path[1] == 0)
		dir = rootdir;
	else
		dir = make_dir(rootdir, path);

        fd = NATIVEOS(open)(path, O_RDONLY);

	if (fd == -1) {
		printf("unix/bmodfs: couldn't open %s, errno = %d\n",
		       path, NATIVEOS(errno));
	} else {
		if (NATIVEOS(fchdir)(fd) == 0) {
			copy_dir(fd, dir);
			NATIVEOS(close)(fd);
			/* XXX fchdir back to where we were */
		} else {
			printf("unix/bmodfs: couldn't fchdir to %s/errno=%d\n",
			       path, NATIVEOS(errno));
		}
	}
	return 0;
}

static oskit_dir_t *
make_dir(oskit_dir_t *dir, const char *path)
{
	char		buf[128], *dp = buf;
	const char 	*bp = path;
	oskit_dir_t	*ddir, *ndir;
	oskit_file_t	*dfile;

	while (*bp && *bp != '/')
		*dp++ = *bp++;

	*dp = '\0';

	oskit_dir_mkdir(dir, buf, 666);
	oskit_dir_lookup(dir, buf, &dfile);
	oskit_file_query(dfile, &oskit_dir_iid, (void**)&ddir);
	oskit_file_release(dfile);

	if (*bp == '/') {
		bp++;
		ndir = make_dir(ddir, bp);
		oskit_dir_release(ddir);
		return ndir;
	}
	return ddir;
}

static void
copy_dir(int dirfd, oskit_dir_t *dst)
{
	static char absname[4096] = "/";
	char buf[1024];
	long base = 0;
	int r;

	while ((r = NATIVEOS(getdirentries)(dirfd, buf, sizeof buf, &base)) > 0)
	{
		struct dirent *d = (struct dirent *)buf;
		while ((void*)d < (void*)(buf + r)) {
			char *name = (char *)malloc(DIRENTNAMLEN(d) + 1);
			struct stat stb;
			int rs;

			memcpy(name, d->d_name, DIRENTNAMLEN(d));
			name[DIRENTNAMLEN(d)] = 0;

			if (!strcmp(name, ".") || !strcmp(name, ".."))
				goto next;

			rs = NATIVEOS(stat)(name, &stb);
			if (rs == -1) {
				printf("unix/bmodfs: can't stat %s/errno = %d, skipping\n",
					name, NATIVEOS(errno));
				goto next;
			}

			if (S_ISDIR(stb.st_mode))
			{
				oskit_dir_t *ddir;
				oskit_file_t *dfile;
				int oldlen = strlen(absname);
				int fd = NATIVEOS(open)(name,
					0 /* don't use O_RDONLY! */);
				strcat(absname, name);
				strcat(absname, "/");
				NATIVEOS(fchdir)(fd);
				oskit_dir_mkdir(dst, name, 666);
				oskit_dir_lookup(dst, name, &dfile);
				oskit_file_query(dfile, &oskit_dir_iid,
					(void**)&ddir);
				oskit_file_release(dfile);
				copy_dir(fd, ddir);
				oskit_dir_release(ddir);
				NATIVEOS(close)(fd);
				NATIVEOS(fchdir)(dirfd);
				absname[oldlen] = 0;
			} else {
				static char fb[65536];
				oskit_error_t rc;
				oskit_file_t *file;
				oskit_openfile_t *ofile;
				oskit_u32_t actual;
				int b, fd = NATIVEOS(open)(name,
					0 /* don't use O_RDONLY!*/);
				if (fd == -1) {
					printf("unix/bmodfs: couldn't open %s/errno=%d\n",
						name, NATIVEOS(errno));
					goto next;
				}
				if (verbose)
					printf("unix/bmodfs: copying %s%s ...\n",
						absname, name);

				rc = oskit_dir_create(dst, name, 0, 666, &file);
				assert(rc == 0);
				/* it's the bmod fs, oskit_file_open would
				 * return rc == 0 & ofile == 0.  Must use
				 * adaptor as in liboskit_c::open().
				 */
				rc = oskit_soa_openfile_from_file(file,
					OSKIT_O_WRONLY, &ofile);
				assert(rc == 0 && ofile);

				while ((b = NATIVEOS(read)
						(fd, fb, sizeof fb)) > 0)
				{
					rc = oskit_openfile_write(ofile,
						fb, b, &actual);
					assert(rc == 0);
					assert(b == actual);
				}
				NATIVEOS(close)(fd);
				oskit_openfile_release(ofile);
				oskit_file_release(file);
			}
		next:
			d = ((void *)d) + d->d_reclen;
			free(name);
		}
	}
}
