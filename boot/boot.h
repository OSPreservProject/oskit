/*
 * Copyright (c) 1996, 1998 University of Utah and the Flux Group.
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

#ifndef _BOOT_BOOT_H_
#define _BOOT_BOOT_H_

#include <oskit/machine/multiboot.h>
#include <oskit/types.h>

extern void *boot_kern_image;
extern struct multiboot_header boot_kern_hdr;

void boot_start(struct multiboot_info *bi);

/*
 * Returns size of extended memory in kilobytes.
 */
oskit_size_t extended_mem_size(void);

#endif /* _BOOT_BOOT_H_ */
