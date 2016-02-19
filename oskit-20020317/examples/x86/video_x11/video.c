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
 * Example program to test the function of the X11 library.
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <malloc.h>
#include <oskit/io/ttystream.h>
#include <oskit/fs/file.h>	/* XXX OSKIT_O_RDWR */
#include <oskit/dev/dev.h>
#include <oskit/dev/tty.h>
#include <oskit/dev/freebsd.h>
#include <oskit/clientos.h>

#include <oskit/io/bufio.h>

#include <oskit/video/fb.h>
#include <oskit/video/s3.h>

#include <oskit/fs/memfs.h>
#include <oskit/c/fs.h>
#include <oskit/startup.h>


int
main(int argc, char *argv[])
{
	int i;
	oskit_addr_t addr, p;
	oskit_fb_t *fb;
	oskit_cmap_entry_t c;

#ifndef KNIT
	oskit_clientos_init();
	start_fs_bmod();
	start_clock();
#endif
	
	fb = (oskit_fb_t *)s3_init_framebuffer();
	if (fb == NULL)
		panic("couldn't init framebuffer!\n");

	oskit_bufio_map(fb->bufio, (void *)&addr, 0, fb->video_mem);
			
	c.index = 0x0f;
	c.red = c.blue = c.red = 0xff;

	s3_cmap_write(&c);

	p = addr;
	for (i = 0; i < 10000; i++) 
		*(char *)p++ = 0x0f;

	getchar();

	return 0;
}

