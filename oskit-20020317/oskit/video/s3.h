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
 * Temporary function prototypes for the s3 video driver.
 */
/*
 * XXX THIS INTERFACE WILL CHANGE!  DO NOT RELY ON IT!
 */
#ifndef _OSKIT_VIDEO_S3_H 
#define _OSKIT_VIDEO_S3_H 

#include <oskit/types.h>

oskit_fb_t *s3_init_framebuffer(void);
int s3_cmap_write(oskit_cmap_entry_t *c);
int s3_cmap_read(oskit_cmap_entry_t *c);
int s3_cmap_fg_index(void);


#endif /* _OSKIT_VIDEO_S3_H */

