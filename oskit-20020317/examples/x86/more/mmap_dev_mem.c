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
 * create a /dev/mem device, mmap it and write a message in the
 * textmode frame buffer
 */

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <oskit/fs/memfs.h>
#include <oskit/c/fs.h>
#include <oskit/startup.h>
#include <oskit/clientos.h>

/*
 * create a /dev/mem device under dev_name, directory for dev_name must exist!
 */
oskit_error_t make_dev_mem(char *dev_name);

void
main()
{
	int 	fd, i;
	char	*screen;
	unsigned short *ss;
	oskit_error_t rc;
	char	*hello = "This is a test of the emergency broadcast system";

	oskit_clientos_init();
	start_fs_bmod();

	mkdir("/dev", 0x775);

	rc = make_dev_mem("/dev/mem");
	assert(rc == 0);

	fd = open("/dev/mem", O_RDWR);
	assert(fd != -1);

	screen = mmap(0, 80*24*2, PROT_READ|PROT_WRITE, MAP_FILE, fd, 0xb8000);
	assert(screen);

	ss = (unsigned short *)screen;
	for (i = 0; i < 24*80; i++) {
		ss[i] = 0xF100 | ' ';
	}
	screen += 12 * 2 * 80 + (80 - strlen(hello));	/* center it */
	for (i = 0; i < strlen(hello); i++) {
		screen[2*i] = hello[i];	/* character */
		screen[2*i+1] = 0x2;	/* attribute: green */
	}
	close(fd);

	/* give user a change to read it... */
	for (fd = 0; fd < 2; fd++) {
		osenv_timer_spin(500000000);
	}
	printf("dev_mem_test exiting\n");
	exit(0);
}
