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
#include <X.h>

#include <xf86.h>
#include <xf86Priv.h>

#include <stdlib.h>
#include <oskit/io/bufio.h>
#include <oskit/video/fb.h>

#include "s3_buf_io.h"
#include "s3_priv.h"

extern ScrnInfoRec s3InfoRec;
extern char *s3VideoMem;


oskit_fb_t *
s3_init_framebuffer(void)
{
	DisplayModePtr p;

	oskit_fb_t *fb;

	fb = (oskit_fb_t *)malloc(sizeof(*fb));
        if (fb == NULL)
                return NULL;

	x11_main();
	p = oskit_s3_init_mode(0);

	fb->bufio = s3_bufio_create((oskit_addr_t)s3VideoMem, 
				    s3InfoRec.videoRam);
	fb->width = p->HDisplay;
	fb->height = p->VDisplay;
	fb->depth = s3InfoRec.bitsPerPixel;
	fb->video_mem = s3InfoRec.videoRam;
	fb->stride = fb->width;				/* XXX */

	atexit(oskit_s3_cleanup);

	return fb;
}


int
s3_cmap_write(oskit_cmap_entry_t *c)
{
	s3WriteDAC(c->index, c->red, c->green, c->blue);
}

int 
s3_cmap_read(oskit_cmap_entry_t *c)
{
	

	s3ReadDAC(c->index, &(c->red), &(c->green), &(c->blue));
	/* Assume an 8bit dac */
	c->maxi = 255;
}

int 
s3_cmap_fg_index(void)
{
	return 0;
}


