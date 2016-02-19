/*
 * Copyright (c) 1996-1998 University of Utah and the Flux Group.
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
 * OSKit fs interface definitions.
 */
#ifndef _OSKIT_FS_FS_H_
#define _OSKIT_FS_FS_H_

#include <oskit/com.h>
#include <oskit/error.h>
#include <oskit/io/posixio.h>

/*
 * External dependencies of the fs library
 *
 *  i486-linux-nm liboskit_netbsd.a | grep ' U ' | \
 *  grep -v fs_netbsd | awk '{print $2}' | sort | uniq
 *
 *  diskpart:
 * 	diskpart_get_partition
 * 	diskpart_lookup_bsd_compat

 *  fdev:	
 * 	osenv_intr_disable
 * 	osenv_intr_enable

 *  com:
 * 	oskit_absio_iid
 * 	oskit_dir_iid
 * 	oskit_file_iid
 * 	oskit_filesystem_iid
 *	oskit_iunknown_iid
 *	oskit_openfile_iid
 *	oskit_posixio_iid
 *	oskit_principal_iid
 *
 *  client OS:
 * 	oskit_get_call_context
 *	fs_delay
 *	fs_free
 *	fs_gettime
 *	fs_malloc
 *	fs_panic	
 *	fs_realloc
 *	fs_tsleep
 *	fs_vprintf
 *	fs_vsprintf
 *	fs_wakeup
 */

void	fs_delay(oskit_u32_t n);

void 	fs_vprintf(const char *, oskit_va_list);

void 	fs_vsprintf(char *, const char *, oskit_va_list);

void 	fs_panic(void);

oskit_error_t fs_gettime(struct oskit_timespec *tsp);

void 	fs_wakeup(void *chan);

oskit_error_t fs_tsleep(void *chan, oskit_u32_t pri, char *wmesg, 
		       oskit_u32_t timo);

void	*fs_malloc(oskit_u32_t size);

void	fs_free(void *addr);

void 	*fs_realloc(void *curaddr, oskit_u32_t newsize);

#endif /* _OSKIT_FS_FS_H_ */
