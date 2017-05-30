/*
 * Copyright (c) 1997-2000 The University of Utah and the Flux Group.
 * 
 * This file is part of the OSKit Linux Glue Libraries, which are free
 * software, also known as "open source;" you can redistribute them and/or
 * modify them under the terms of the GNU General Public License (GPL),
 * version 2, as published by the Free Software Foundation (FSF).
 * 
 * The OSKit is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GPL for more details.  You should have
 * received a copy of the GPL along with the OSKit; see the file COPYING.  If
 * not, write to the FSF, 59 Temple Place #330, Boston, MA 02111-1307, USA.
 */
/*
 * The definitions of types derived from the oskit_filesystem_t,
 * oskit_file_t, oskit_dir_t, types.
 * They are prefixed with a `g' for `glue'.
 */

#ifndef _LINUX_FS_TYPES_H_
#define _LINUX_FS_TYPES_H_

#include <oskit/fs/filesystem.h>
#include <oskit/fs/file.h>
#include <oskit/fs/dir.h>
#include <oskit/fs/openfile.h>
#include <oskit/io/absio.h>

#include <linux/fs.h>

/*
 * Our filesystem representation, "inherited" from oskit_filesystem_t.
 */
struct gfilesystem {
	oskit_filesystem_t fsi;		/* COM filesystem inteface */
	unsigned count;			/* ref count */
	struct super_block *sb;		/* Linux super block representation */
};
typedef struct gfilesystem gfilesystem_t;

/*
 * This is to emphasize that gdir_t is a subset of gfile_t.
 */
#define __COMMON_FILE_GLUE						\
	oskit_absio_t absioi;		/* COM absio interface */	\
	unsigned count;			/* ref count */			\
	struct dentry *dentry;		/* Linux cache entry */		\
	gfilesystem_t *fs;		/* containing filesytem */

/*
 * Our file representation, "inherited" from oskit_file_t.
 */
struct gfile {
	oskit_file_t filei;		/* COM file interface */
	__COMMON_FILE_GLUE
};
typedef struct gfile gfile_t;

extern oskit_error_t gfile_create(gfilesystem_t *fs, struct dentry *dentry,
				  gfile_t **out_file);

extern oskit_error_t gfile_lookup(gfilesystem_t *fs, struct dentry *dentry,
				  gfile_t **out_file);

/*
 * Our directory representation, "inherited" from oskit_dir_t.
 * This must be a subset of gfile_t.
 */
struct gdir {
	oskit_dir_t diri;		/* COM directory interface */
	__COMMON_FILE_GLUE
	oskit_dir_t *vparent;		/* virtual parent dir; NULL if
					   haven't been reparented */
};
typedef struct gdir gdir_t;

/*
 * Our openfile representation, "inherited" from oskit_openfile_t.
 */
struct gopenfile {
	oskit_openfile_t ofilei;	/* COM openfile interface */
	oskit_absio_t absioi;		/* COM absio interface */
	unsigned count;			/* ref count */
	gfile_t *gf;			/* associated gfile */
	struct file *filp;		/* Linux file representation */
};
typedef struct gopenfile gopenfile_t;

extern oskit_error_t gopenfile_create(gfile_t *file, struct file *f,
				      gopenfile_t **out_openfile);

/*
 * Handy macros for testing objects.
 */
#define VERIFY_OR_PANIC(obj, name) ({				\
	if ((obj) == NULL)					\
		panic(__FUNCTION__": null " #name);		\
	if ((obj)->count == 0)					\
		panic(__FUNCTION__": bad count");		\
})
static inline 
int verify_fs(gfilesystem_t *obj, void* non_null_data)
{
	return (obj != NULL 
                && obj->count != 0 
                && non_null_data != NULL);
}
static inline 
int verify_file(gfile_t *file)
{
	return (file != NULL 
                && file->count != 0	
                && file->dentry != NULL
                && file->fs != NULL);
}
static inline 
int verify_openfile(gopenfile_t *ofile)
{
	return (ofile != NULL 
                && ofile->count != 0	
                && ofile->gf != NULL
                && ofile->filp != NULL);
}
static inline 
int verify_dir(gdir_t *dir)
{
	return (dir != NULL 
                && dir->count != 0	
                && dir->dentry != NULL
                && dir->fs != NULL);
}

#endif /* _LINUX_FS_TYPES_H_ */

