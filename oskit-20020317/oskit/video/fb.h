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
 * Definition of an extended buffer-oriented I/O interface,
 * which extends the basic absio and blkio interfaces.
 */
/*
 * XXX THIS INTERFACE WILL CHANGE!  DO NOT RELY ON IT!
 */
#ifndef _OSKIT_VIDEO_FB_H 
#define _OSKIT_VIDEO_FB_H 

#include <oskit/types.h>
#include <oskit/io/bufio.h>


struct oskit_fb {
	oskit_bufio_t	*bufio;
	int		width, height, depth, stride;	
	int		video_mem;
};
typedef struct oskit_fb oskit_fb_t;


struct oskit_cmap_entry {
	int	index, red, green, blue, maxi;
};
typedef struct oskit_cmap_entry oskit_cmap_entry_t;


#endif /* _OSKIT_VIDEO_FB_H */
