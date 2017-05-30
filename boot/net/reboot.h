/*
 * Copyright (c) 1998 University of Utah and the Flux Group.
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

#ifndef _BOOT_NET_REBOOT_H_
#define _BOOT_NET_REBOOT_H_

#ifndef ASSEMBLER

/*
 * Structure describing where the stashed kernel is,
 * how big it is,
 * and how to call it.
 *
 * The magicloc field deserves some explanation.
 * When netboot is started it stashes itself up in high memory
 * and modifies the clone's copy of the reboot stub to access
 * a reboot_info struct.
 * It finds the area in the stub to modify by looking for REBOOT_INFO_MAGIC
 * and replacing it with the address of the reboot_info struct.
 * When the stub runs, it stores the reboot_info address in a register
 * and then unmodifies itself before copying the kernel back down
 * to it's linked address.
 * This way the netboot startup code can again find REBOOT_INFO_MAGIC in
 * the stub.
 * So, the reboot_info.magicloc field is the address of the place in the
 * stub to modify (the address is pointing into the clone space).
 */
struct reboot_info {
	oskit_addr_t src;		/* 0: where it is */
	oskit_size_t size;		/* 4: how big it is */
	oskit_addr_t arg;		/* 8: what to pass */
	oskit_addr_t magicloc;		/* 12: see above */
};

extern void reboot(void);	/* reboot.S */
extern oskit_addr_t reboot_stub_loc;

extern void patch_clone(void* what, int size);

#endif /* ASSEMBLER */

#define REBOOT_INFO_MAGIC	0xfacebeef

#endif /* _BOOT_NET_REBOOT_H_ */
