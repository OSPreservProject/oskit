/*
 * Copyright (c) 1996-1998, 2000, 2001 University of Utah and the Flux Group.
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
#ifndef __BOOT_H_INCLUDED__
#define __BOOT_H_INCLUDED__

#include <oskit/x86/multiboot.h>	/* multiboot_header */
#include <sys/types.h>
#include <netinet/in.h>			/* in_addr */

struct kerninfo {
	struct in_addr ki_ip;		/* ip of server host, in host order */
	unsigned ki_transport;		/* transport protocol */
	char *ki_dirname;		/* dir to mount */
	char *ki_filename;		/* file within dir */

	struct multiboot_header ki_mbhdr; /* things like load address, entry */

	void *ki_imgaddr;		  /* where it is in memory */

	struct multiboot_info *ki_mbinfo;  /* stuff we are passing to it */
};

/* ki_transport values */
#define XPORT_NFS	0
#define XPORT_TFTP	1

#define kerninfo_imagesize(ki)	(phystokv((ki)->ki_mbhdr.bss_end_addr) \
				 - phystokv((ki)->ki_mbhdr.load_addr))

int getkernel_net(struct kerninfo *ki /* IN/OUT */);
void loadkernel(struct kerninfo *ki);

#endif /* __BOOT_H_INCLUDED__ */
