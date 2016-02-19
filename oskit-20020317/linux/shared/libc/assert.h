/*
 * Copyright (c) 1995, 1997, 1998 The University of Utah and the Flux Group.
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
#ifndef _LIBC_ASSERT_H_
#define _LIBC_ASSERT_H_

#ifdef NDEBUG

#define assert(ignore) ((void)0)

#else

extern void panic(const char *format, ...);

#define assert(expression)  \
	((void)((expression) ? 0 : \
		(panic(__FILE__":%u: failed assertion `"#expression"'", \
			__LINE__), 0)))

#endif

#endif /* _LIBC_ASSERT_H_ */
