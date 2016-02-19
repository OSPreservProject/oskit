#ifndef _NETBSD_OSKIT_FS_GLUE_H_
#define _NETBSD_OSKIT_FS_GLUE_H_

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

#include <oskit/boolean.h>
#include <oskit/io/absio.h>
#include <oskit/com/stream.h>
#include <oskit/fs/filesystem.h>
#include <oskit/fs/file.h>
#include <oskit/fs/dir.h>
#include <oskit/fs/openfile.h>
#include <oskit/c/assert.h>

#include "getproc.h"
#include "osenv.h"

#include <sys/uio.h>
#include <sys/dirent.h>
#include <sys/syscallargs.h>
#include <oskit/fs/fs.h>

#include "errno.h"
#include "hashtab.h"


struct gchildfs
{
    oskit_filesystem_t *fs;	/* child file system */
    unsigned count;		/* reference count */
    struct gchildfs *next;	/* next child */
};


struct gfilesystem
{
    oskit_filesystem_t fsi;	/* COM filesystem interface */
    unsigned count;		/* reference count */
    struct mount *mp;		/* filesystem mount */
};


struct file_glue {
    oskit_absio_t absioi;	/* COM absolute I/O interface */
    unsigned count;		/* reference count */
    struct gfilesystem *fs;	/* containing file system */
    struct vnode *vp;		/* file vnode */
};

struct gfile {
	oskit_file_t filei;		/* COM file interface */
	struct file_glue f;
};


struct gdir
{
	oskit_dir_t diri;	/* COM directory interface */
	struct file_glue f;	/* file object data for the directory */
	oskit_dir_t *parent;	/* logical parent; null if a root directory */
	oskit_bool_t virtual;	/* virtual directory; use logical parent */
};


struct gopenfile
{
    oskit_openfile_t ofilei;	/* COM open file interface */
    oskit_absio_t absioi;	/* COM absolute I/O interface */
    unsigned count;		/* reference count */
    struct gfile  *file;	/* associated file */
    struct file *fp;		/* file description */
};



struct gfilesystem *gfilesystem_create(struct mount *mp);

oskit_error_t gfilesystem_mount(struct gfilesystem *fs,
			       oskit_filesystem_t *childfs);

void gfilesystem_unmount(struct gfilesystem *fs, oskit_filesystem_t *childfs);

struct gfile *gfile_create(struct gfilesystem *fs, struct vnode *vp);

struct gopenfile *gopenfile_create(struct gfile *file, struct file *fp);

#define realloc fs_netbsd_realloc
void *realloc(void *curaddr, unsigned long newsize);

extern hashtab_t vptab;  	/* vp -> oskit_file_t */

/*
 * Macros to assist in state save across calls out of the component.
 */
#define SSTATE_DECL \
    struct proc *saved_p; \
    int saved_spl;

#define SSTATE_SAVE \
    saved_p   = curproc; \
    saved_spl = reset_spl();

#define SSTATE_RESTORE \
    restore_spl(saved_spl); \
    curproc   = saved_p;

/*
 * Make sure SPL is reset while sleeping.  Since it is global, another
 * process-level thread could start with this CPL, instead of starting
 * where it left off..
 *
 * Since curproc is global, it too must be properly saved and restored
 * across calls to osenv_sleep.
 */
#define OSKIT_NETBSD_SLEEP(proc) ({					\
	unsigned __spl;							\
	int      __flag;						\
									\
	osenv_sleep_init(&(proc)->p_sr);				\
	__spl = reset_spl();						\
	__flag = osenv_sleep(&(proc)->p_sr);				\
	curproc = (proc);						\
	restore_spl(__spl);						\
	__flag; })

#endif /* _NETBSD_OSKIT_FS_GLUE_H_ */
