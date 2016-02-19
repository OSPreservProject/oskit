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
 * Declarations for some simple object adapters provided by -loskit_com.
 *
 * Note that the "soa" component in the names are obscure and historical,
 * and will be purged some day.
 * 
 */

#ifndef _OSKIT_FS_SOA_H_
#define _OSKIT_FS_SOA_H_

#include <oskit/fs/file.h>
#include <oskit/fs/openfile.h>
#include <oskit/io/absio.h>
#include <oskit/com/stream.h>
#include <oskit/dev/blk.h>
#include <oskit/dev/tty.h>
#include <oskit/dev/clock.h>

/*
 * Create a `oskit_openfile' adapter object wrapped around a `oskit_file'
 * object that optionally supports the `oskit_absio' or `oskit_blkio' protocol
 * for random-access i/o; if the given absio pointer is null,
 * queries the given file for `oskit_absio' and fails if it's not supported.
 */
OSKIT_COMDECL oskit_soa_openfile_from_absio(oskit_file_t *f,
					  oskit_absio_t *aio,
					  oskit_oflags_t flags,
					  oskit_openfile_t **out_openfile);


/*
 * Create a `oskit_openfile' adapter object wrapped around a `oskit_file'
 * object that optionally supports the `oskit_stream' protocol for i/o;
 * if the given stream pointer is non-null, stream operations go there;
 * otherwise stream operations will fail on the returned `oskit_openfile'.
 */
OSKIT_COMDECL oskit_soa_openfile_from_stream(oskit_file_t *f,
					   oskit_stream_t *stream,
					   oskit_oflags_t flags,
					   oskit_openfile_t **out_openfile);

/*
 * An 'openfile from read/write/lseek' adapter.
 * The resulting openfile object will refuse any calls but calls to
 * read, write, lseek, and getfile. The first three are bounced to the
 * callbacks, the last one returns the `file' we passed in.
 */
OSKIT_COMDECL
oskit_soa_openfile_from_rwl(
        oskit_file_t            *f,
        oskit_oflags_t          flags,
	OSKIT_COMDECL (*read)(
			void *cookie,
                	void *buf,
			oskit_u32_t len,
			oskit_u32_t *out_actual),
	OSKIT_COMDECL (*write)(
			void *cookie,
                	const void *buf,
			oskit_u32_t len,
			oskit_u32_t *out_actual),
	OSKIT_COMDECL (*seek)(
			void *cookie,
                	oskit_s64_t offset,
			oskit_seek_t whence,
                	oskit_u64_t *out_newpos),
        void            *cookie,
        struct oskit_openfile **out_openfile);

/*
 * Creates a `oskit_openfile' adapter object wrapped around
 * an arbitrary `oskit_file' object.
 * This function automatically detects what interfaces the file object supports
 * and uses the appropriate one of the above adaptors.
 */
OSKIT_COMDECL oskit_soa_openfile_from_file(oskit_file_t *f,
					 oskit_oflags_t flags,
					 oskit_openfile_t **out_openfile);


/*
 * Create a `oskit_file' adapter object wrapped around a `oskit_blkdev' object.
 */
OSKIT_COMDECL oskit_soa_file_from_blk(oskit_blkdev_t *blkdev,
				      oskit_file_t **out_file);

/*
 * Create a `oskit_file' adapter object wrapped around a `oskit_ttydev' object.
 */
OSKIT_COMDECL oskit_soa_file_from_tty(oskit_ttydev_t *ttydev,
				      oskit_file_t **out_file);


/*
 * Create a `oskit_file' adapter object wrapped around a `oskit_clock' object.
 */
OSKIT_COMDECL oskit_soa_file_from_clock(oskit_clock_t *clock,
					oskit_file_t **out_file);

/*
 * Create a `oskit_file' adapter object which will call `open' on open
 * and `close' when released.
 */
OSKIT_COMDECL
oskit_soa_file_universal(
    OSKIT_COMDECL (*open)(oskit_file_t *file,
			  void *cookie,
			  oskit_oflags_t,
			  oskit_openfile_t **),
    OSKIT_COMDECL_V (*close)(void *cookie),
    void *cookie,
    struct oskit_stat *stat,
    oskit_file_t **out_file);

#endif /* _OSKIT_FS_SOA_H_ */
