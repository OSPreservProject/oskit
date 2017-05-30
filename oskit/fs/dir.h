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
 * Definition of the COM oskit_dir interface,
 * which represents a POSIX.1 directory object.
 */
#ifndef _OSKIT_FS_DIR_H_
#define _OSKIT_FS_DIR_H_

#include <oskit/io/posixio.h>
#include <oskit/fs/file.h>
#include <oskit/fs/dirents.h>

struct oskit_filesystem;
struct oskit_openfile;


/*
 * Basic open-directory object interface,
 * IID 4aa7df95-7c74-11cf-b500-08000953adc2.
 * This interface extends the oskit_file interface.
 */
struct oskit_dir {
	struct oskit_dir_ops *ops;
};
typedef struct oskit_dir oskit_dir_t;

struct oskit_dir_ops {

	/*** Operations inherited from oskit_unknown interface ***/
	OSKIT_COMDECL	(*query)(oskit_dir_t *d,
				 const struct oskit_guid *iid,
				 void **out_ihandle);
	OSKIT_COMDECL_U	(*addref)(oskit_dir_t *d);
	OSKIT_COMDECL_U	(*release)(oskit_dir_t *d);

	/*** Operations inherited from oskit_posixio_t ***/
	OSKIT_COMDECL	(*stat)(oskit_dir_t *d,
				oskit_stat_t *out_stats);
	OSKIT_COMDECL	(*setstat)(oskit_dir_t *d, oskit_u32_t mask,
				   const oskit_stat_t *stats);
	OSKIT_COMDECL	(*pathconf)(oskit_dir_t *d, oskit_s32_t option,
				    oskit_s32_t *out_val);


	/*** Operations inherited from oskit_file interface ***/
	OSKIT_COMDECL	(*sync)(oskit_dir_t *d, oskit_bool_t wait);
	OSKIT_COMDECL	(*datasync)(oskit_dir_t *d, oskit_bool_t wait);
	OSKIT_COMDECL	(*access)(oskit_dir_t *d, oskit_amode_t mask);
	OSKIT_COMDECL 	(*readlink)(oskit_dir_t *d, 
				    char *buf, oskit_u32_t len, 
				    oskit_u32_t *out_actual);
	OSKIT_COMDECL	(*open)(oskit_dir_t *d, oskit_oflags_t flags,
				struct oskit_openfile **out_opendir);
	OSKIT_COMDECL	(*getfs)(oskit_dir_t *d, 
				 struct oskit_filesystem **out_fs);


	/*** Operations derived from POSIX.1-1990 ***/

	/*
	 * Look up the node with the specified name in this directory.
	 * If successful, returns a reference to the node in 'out_file'.
	 *
	 * The name may only be a single component; multi-component
	 * lookups are not supported.
	 *
	 * If the file is a symbolic link, then 'out_file' will reference
	 * the symbolic link itself.
	 */
	OSKIT_COMDECL	(*lookup)(oskit_dir_t *d, const char *name,
				  oskit_file_t **out_file);

	/*
	 * Same as lookup, except that if the file does not exist,
	 * then a regular file will be created with the specified 
	 * name and mode.
	 *
	 * The name may only be a single component; multi-component
	 * lookups are not supported.
	 *
	 * If the file exists and is a symbolic link, 
	 * then 'out_file' will reference the symbolic link itself.
	 *
	 * Returns OSKIT_EEXIST if 'excl' is true and the file already exists.
	 */
	OSKIT_COMDECL	(*create)(oskit_dir_t *d, const char *name,
				  oskit_bool_t excl, oskit_mode_t mode,
				  oskit_file_t **out_file);

	/*
	 * Link a file into the file system under this directory.
	 * The new 'name' is the name of the new entry to create,
	 * and 'file' is the file to link in.
	 *
	 * Generally fails if 'file' is on a different file system
	 * as the directory under which it is to be linked.
	 *
	 * The name may only be a single component; multi-component
	 * lookups are not supported.
	 *
	 * 'file' may not be a symbolic link.
	 */
	OSKIT_COMDECL	(*link)(oskit_dir_t *d, const char *name,
				oskit_file_t *file);

	/*
	 * Remove the named directory entry.
	 *
	 * The name may only be a single component; multi-component
	 * lookups are not supported.
	 */
	OSKIT_COMDECL	(*unlink)(oskit_dir_t *d, const char *name);

	/*
	 * Atomically rename file 'old_name' under 'old_dir'
	 * to 'new_name' under this directory, 'new_dir'.
	 *
	 * If a file named 'new_name' already exists in 'new_dir', 
	 * then it is first removed.  In this case, the source and target files
	 * must either both be directories or both be non-directories,
	 * and if the target file is a directory, it must be empty.
	 *
	 * The names may each only be a single component; multi-component
	 * lookups are not supported.
	 *
	 * Typically, this is only supported if 'new_dir'
	 * resides in the same filesystem as 'old_dir'.
	 */
	OSKIT_COMDECL	(*rename)(oskit_dir_t *old_dir, const char *old_name,
			 	  oskit_dir_t *new_dir, const char *new_name);

	/*
	 * Create a new subdirectory in this directory,
	 * with the specified permissions.
	 *
	 * The name may only be a single component; multi-component
	 * lookups are not supported.
	 */
	OSKIT_COMDECL	(*mkdir)(oskit_dir_t *d, const char *name,
				 oskit_mode_t mode);

	/*
	 * Remove the named subdirectory from this directory.
	 * Generally fails if the subdirectory is non-empty.
	 *
	 * The name may only be a single component; multi-component
	 * lookups are not supported.
	 */
	OSKIT_COMDECL	(*rmdir)(oskit_dir_t *d, const char *name);

	/*
	 * Read one or more entries in this directory.
	 * The offset of the entry to be read is specified in 'inout_ofs';
	 * the caller must initially supply zero as the offset,
	 * and during each successful call the offset increases
	 * by an unspecified amount dependent on the file system.
	 * The file system will return at least 'nentries'
	 * if there are at least this many remaining in the directory;
	 * however, the file system may return more entries.
	 *
	 * The caller must free the dirents array and the names
	 * using the task allocator when they are no longer needed.
	 */
	OSKIT_COMDECL	(*getdirentries)(oskit_dir_t *d,
					 oskit_u32_t *inout_ofs,
					 oskit_u32_t nentries,
					 oskit_dirents_t **out_dirents);

	/*** X/Open CAE 1994 ***/

	/*
	 * Create a special file node.
	 *
	 * The 'name' may only be a single component; 
	 * multi-component lookups are not supported.
	 */
	OSKIT_COMDECL	(*mknod)(oskit_dir_t *d, const char *name,
				 oskit_mode_t mode, oskit_dev_t dev);

	/*
	 * Create a symbolic link.	
	 *
	 * The 'link_name' may only be a single component; 
	 * multi-component lookups are not supported.
	 */
	OSKIT_COMDECL	(*symlink)(oskit_dir_t *d, const char *link_name,
				   const char *dest_name);

	/*** Operations for composing file system namespaces ***/

	/*
	 * Create a new "virtual" directory object
	 * referring to the same underlying directory as this one,
	 * but whose logical parent directory
	 * (the directory named by the '..' entry in this directory)
	 * is the specified directory node handle.
	 *
	 * If 'parent' is NULL, lookups of the '..' entry in the 
	 * virtual directory object will return a reference to
	 * the virtual directory object itself; as with physical
	 * root directories, virtual root directories will have
	 * self-referential '..' entries.
	 */
	OSKIT_COMDECL	(*reparent)(oskit_dir_t *d, oskit_dir_t *parent,
				    oskit_dir_t **out_dir);
};

#define oskit_dir_query(d, iid, out_ihandle) \
	((d)->ops->query((oskit_dir_t *)(d), (iid), (out_ihandle)))
#define oskit_dir_addref(d) \
	((d)->ops->addref((oskit_dir_t *)(d)))
#define oskit_dir_release(d) \
	((d)->ops->release((oskit_dir_t *)(d)))
#define oskit_dir_stat(d, out_stats) \
	((d)->ops->stat((oskit_dir_t *)(d), (out_stats)))
#define oskit_dir_setstat(d, mask, stats) \
	((d)->ops->setstat((oskit_dir_t *)(d), (mask), (stats)))
#define oskit_dir_pathconf(d, option, out_val) \
	((d)->ops->pathconf((oskit_dir_t *)(d), (option), (out_val)))
#define oskit_dir_sync(d, wait) \
	((d)->ops->sync((oskit_dir_t *)(d), (wait)))
#define oskit_dir_datasync(d, wait) \
	((d)->ops->datasync((oskit_dir_t *)(d), (wait)))
#define oskit_dir_access(d, mask) \
	((d)->ops->access((oskit_dir_t *)(d), (mask)))
#define oskit_dir_readlink(d, buf, len, out_actual) \
	((d)->ops->readlink((oskit_dir_t *)(d), (buf), (len), (out_actual)))
#define oskit_dir_open(d, flags, out_opendir) \
	((d)->ops->open((oskit_dir_t *)(d), (flags), (out_opendir)))
#define oskit_dir_getfs(d, out_fs) \
	((d)->ops->getfs((oskit_dir_t *)(d), (out_fs)))
#define oskit_dir_lookup(d, name, out_file) \
	((d)->ops->lookup((oskit_dir_t *)(d), (name), (out_file)))
#define oskit_dir_create(d, name, excl, mode, out_file) \
	((d)->ops->create((oskit_dir_t *)(d), (name), (excl), (mode), (out_file)))
#define oskit_dir_link(d, name, file) \
	((d)->ops->link((oskit_dir_t *)(d), (name), (file)))
#define oskit_dir_unlink(d, name) \
	((d)->ops->unlink((oskit_dir_t *)(d), (name)))
#define oskit_dir_rename(old_dir, old_name, new_dir, new_name) \
	((old_dir)->ops->rename((oskit_dir_t *)(old_dir), (old_name), (new_dir), (new_name)))
#define oskit_dir_mkdir(d, name, mode) \
	((d)->ops->mkdir((oskit_dir_t *)(d), (name), (mode)))
#define oskit_dir_rmdir(d, name) \
	((d)->ops->rmdir((oskit_dir_t *)(d), (name)))
#define oskit_dir_getdirentries(d, inout_ofs, nentries, out_dirents) \
	((d)->ops->getdirentries((oskit_dir_t *)(d), (inout_ofs), (nentries), (out_dirents)))
#define oskit_dir_mknod(d, name, mode, dev) \
	((d)->ops->mknod((oskit_dir_t *)(d), (name), (mode), (dev)))
#define oskit_dir_symlink(d, link_name, dest_name) \
	((d)->ops->symlink((oskit_dir_t *)(d), (link_name), (dest_name)))
#define oskit_dir_reparent(d, parent, out_dir) \
	((d)->ops->reparent((oskit_dir_t *)(d), (parent), (out_dir)))

/* GUID for oskit_dir interface */
extern const struct oskit_guid oskit_dir_iid;
#define OSKIT_DIR_IID OSKIT_GUID(0x4aa7df95, 0x7c74, 0x11cf, \
			0xb5, 0x00, 0x08, 0x00, 0x09, 0x53, 0xad, 0xc2)


#endif /* _OSKIT_FS_DIR_H_ */
