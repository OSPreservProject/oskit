/*
 * Copyright (c) 1996, 1997, 1998 The University of Utah and the Flux Group.
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
 * Selective/carpetbombing printfery.
 */

#ifndef _LINUX_DEV_NET_DEBUG_H_
#define _LINUX_DEV_NET_DEBUG_H_

#define DEBUG 1
#ifdef DEBUG

extern unsigned net_debug_mode;

# define D_NONE		0
# define D_ALL		0xffffffff
# define D_INIT		(1<<0)
# define D_VINIT	(1<<1)
# define D_RECV		(1<<2)
# define D_VRECV	(1<<3)
# define D_SEND		(1<<4)
# define D_VSEND	(1<<5)

# define DPRINTF(when, fmt, args...) ({				\
	if (net_debug_mode & (when))				\
		osenv_log(OSENV_LOG_DEBUG, "%s:" fmt, __FUNCTION__ , ## args); \
})

# define DEXEC(when, what) ({					\
	if (net_debug_mode & (when))				\
		{ what; }					\
})

#else /* not DEBUG */

# define DPRINTF(when, fmt, args...)
# define DEXEC(when, what)

#endif /* DEBUG */

#endif /* _LINUX_DEV_NET_DEBUG_H_ */
