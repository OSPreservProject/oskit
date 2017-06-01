/*
 * Copyright (c) 2001 University of Utah and the Flux Group.
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
 * Example program to test sound card support
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <oskit/clientos.h>
#include <oskit/startup.h>
#include <oskit/dev/linux.h>
#include <oskit/dev/osenv.h>

extern int oskit_linux_sound_open(const char *path, int flags);
extern int oskit_linux_sound_write(int fd, const char *buf, int count);
extern int oskit_linux_sound_close(int fd);

char buf[64*1024];

int
main(int argc, char *argv[])
{
	int infd, outfd, cc;

#ifndef KNIT
	oskit_clientos_init();
	start_sound_devices();
	start_fs_bmod();
#endif

	infd = open("soundfile", 0);
	if (infd < 0)
		panic("No file \"soundfile\" to play!");

	outfd = oskit_linux_sound_open("/dev/audio", O_WRONLY);
	if (outfd < 0)
		panic("Cannot open audio device!");

	printf("playing soundfile...\n");
	while ((cc = read(infd, buf, sizeof buf)) > 0) {
		cc = oskit_linux_sound_write(outfd, buf, cc);
		if (cc < 0) {
			printf("audio output error\n");
			break;
		}
	}

#if 0
	/* XXX avoid cache problems */
	memset(buf, 0x80, sizeof buf);
	oskit_linux_sound_write(outfd, buf, sizeof buf);
#endif

	printf("done, closing device...\n");
	oskit_linux_sound_close(outfd);
	close(infd);
	printf("all done!\n");
	return 0;
}

