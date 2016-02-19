/*
 * Copyright (c) 1997-1998 University of Utah and the Flux Group.
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
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <oskit/com.h>
#include <oskit/com/wrapper.h>
#include <oskit/c/string.h>
#include <oskit/boolean.h>
#include <oskit/io/absio.h>
#include <oskit/com/stream.h>
#include <oskit/fs/filesystem.h>
#include <oskit/fs/file.h>
#include <oskit/fs/dir.h>
#include <oskit/fs/openfile.h>
#include <oskit/c/assert.h>
#include <oskit/c/malloc.h>

/*
 * Wrappers for oskit_filesysem component.
 */
struct gfilesystem {
	oskit_filesystem_t	fsi;		/* COM file interface */
	int			count;
	oskit_filesystem_t	*w_fsi;		/* Wrapped COM FS interface */
	void			(*before)(void *);
	void			(*after)(void *);
	void			*cookie;
};

#define WRAPPERCALL(g, ops, iface, call, args...) ({			      \
	oskit_error_t __err;						      \
	g->before(g->cookie);						      \
	__err = oskit_##iface##_##call(g->w_##ops ,##args);        	      \
	DEBUGWRAPPER(iface, call, __err);				      \
	g->after(g->cookie);						      \
	__err; })

#define WRAPPERCALL_NOARGS(g, ops, iface, call) ({			      \
	oskit_error_t __err;						      \
	g->before(g->cookie);						      \
	__err = oskit_##iface##_##call(g->w_##ops);	      	      	      \
	DEBUGWRAPPER(iface, call, __err);				      \
	g->after(g->cookie);						      \
	__err; })


/*
 * oskit_filesystem methods
 */
static OSKIT_COMDECL
filesystem_query(oskit_filesystem_t *f,
		 const struct oskit_guid *iid, void **out_ihandle)
{
	struct gfilesystem	*fs = (struct gfilesystem *) f;

	if (!fs || !fs->count)
		return OSKIT_E_INVALIDARG;

	if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
	    memcmp(iid, &oskit_filesystem_iid, sizeof(*iid)) == 0) {
		*out_ihandle = &fs->fsi;
		fs->before(fs->cookie);
		++fs->count;
		fs->after(fs->cookie);
		return 0;
	}

	*out_ihandle = NULL;
	return OSKIT_E_NOINTERFACE;    
}

static OSKIT_COMDECL_U
filesystem_addref(oskit_filesystem_t *f)
{
	struct gfilesystem	*fs = (struct gfilesystem *) f;
	unsigned		newcount;

	if (!fs || !fs->count)
		return OSKIT_E_INVALIDARG;

	fs->before(fs->cookie);
	newcount = ++fs->count;
	fs->after(fs->cookie);

	return newcount;
}

static OSKIT_COMDECL_U
filesystem_release(oskit_filesystem_t *f)
{
	struct gfilesystem	*fs = (struct gfilesystem *) f;
        unsigned		newcount;

	if (!fs || !fs->count)
		return OSKIT_E_INVALIDARG;

	fs->before(fs->cookie);
	if ((newcount = --fs->count) == 0) {
		if (fs->w_fsi)
			oskit_filesystem_release(fs->w_fsi);
		fs->after(fs->cookie);
		free(fs);
	}
	else
		fs->after(fs->cookie);
		
	return newcount;
}

static OSKIT_COMDECL
filesystem_statfs(oskit_filesystem_t *f, oskit_statfs_t *out_stats)
{
	struct gfilesystem	*fs = (struct gfilesystem *) f;
	oskit_error_t		err;

	if (!fs || !fs->count)
		return OSKIT_E_INVALIDARG;

	err = WRAPPERCALL(fs, fsi, filesystem, statfs, out_stats);

	return err;
}

static OSKIT_COMDECL
filesystem_sync(oskit_filesystem_t *f, oskit_bool_t wait)
{
	struct gfilesystem	*fs = (struct gfilesystem *) f;
	oskit_error_t		err;

	if (!fs || !fs->count)
		return OSKIT_E_INVALIDARG;

	err = WRAPPERCALL(fs, fsi, filesystem, sync, wait);

	return err;
}

static OSKIT_COMDECL
filesystem_getroot(oskit_filesystem_t *f, struct oskit_dir **out_dir)
{
	struct gfilesystem	*fs = (struct gfilesystem *) f;
	oskit_dir_t		*newdir;
	oskit_error_t		err;

	if (!fs || !fs->count)
		return OSKIT_E_INVALIDARG;

	err = WRAPPERCALL(fs, fsi, filesystem, getroot, &newdir);

	if (err == 0) {
		err = oskit_wrap_dir(newdir,
				     fs->before, fs->after, fs->cookie,
				     out_dir);

		if (err == 0)
			oskit_dir_release(newdir);
	}

	return err;
}

static OSKIT_COMDECL
filesystem_remount(oskit_filesystem_t *f, oskit_u32_t flags)
{
	struct gfilesystem	*fs = (struct gfilesystem *) f;
	oskit_error_t		err;

	if (!fs || !fs->count)
		return OSKIT_E_INVALIDARG;

	err = WRAPPERCALL(fs, fsi, filesystem, remount, flags);

	return err;
}

static OSKIT_COMDECL
filesystem_unmount(oskit_filesystem_t *f)
{
	struct gfilesystem	*fs = (struct gfilesystem *) f;
	oskit_error_t		err;

	if (!fs || !fs->count)
		return OSKIT_E_INVALIDARG;

	err = WRAPPERCALL(fs, fsi, filesystem, unmount);

	return err;
}

static struct oskit_filesystem_ops fs_ops = {
	filesystem_query,
	filesystem_addref,
	filesystem_release,
	filesystem_statfs,
	filesystem_sync,
	filesystem_getroot,
	filesystem_remount,
	filesystem_unmount
};

/*
 * oskit_filesystem wrapper constructor.
 */
oskit_error_t
oskit_wrap_filesystem(oskit_filesystem_t *in,
		      void (*before)(),
		      void (*after)(),
		      void *cookie,
		      oskit_filesystem_t **out)
{
	struct gfilesystem	*newfs = malloc(sizeof(*newfs));

        if (newfs == NULL)
                return OSKIT_ENOMEM;
        memset(newfs, 0, sizeof(*newfs));

        newfs->count   = 1;
        newfs->fsi.ops = &fs_ops;
 	newfs->w_fsi   = in;
	oskit_filesystem_addref(in);

	newfs->before = before;
	newfs->after  = after;
	newfs->cookie = cookie;

	*out = &newfs->fsi;
	return 0;
}


    

