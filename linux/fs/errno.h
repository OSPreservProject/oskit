/*
 * Copyright (c) 1997, 1998 The University of Utah and the Flux Group.
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
#ifndef _LINUX_FS_ERRNO_H_
#define _LINUX_FS_ERRNO_H_

#include <oskit/error.h>
#include <linux/errno.h>

extern oskit_error_t linux_errno_to_oskit_error[EDQUOT];

#define errno_to_oskit_error(e) (((e) == 0) ? 0 : \
				((e) > 0 && (e) <= EDQUOT) ? \
				linux_errno_to_oskit_error[(e)-1] : \
				OSKIT_E_FAIL)

#endif /* _LINUX_FS_ERRNO_H_ */
