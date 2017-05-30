/*
 * Copyright (c) 2000, 2001 University of Utah and the Flux Group.
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
#ifndef _BOOT_PXE_DECLS_H_
#define _BOOT_PXE_DECLS_H_

/*
 * Convert physical address to real mode segment/offset.
 * Since pxeboot sits in the lower 64k, any address can be described
 * using just the offset portion.
 */
#define PXESEG(b)	(oskit_u16_t)0
#define PXEOFF(b)	(oskit_u16_t)((oskit_u32_t)(b))

/*
 * Trampoline to the PXE.
 */
void
pxe_invoke(int routine, void *data);

/*
 * Get ready to boot a real kernel.
 */
void
pxe_cleanup(void);

/*
 * Our DHCP information, taken from the PXE.
 */
extern struct in_addr		myip, serverip, gatewayip;


/*
 * Load a multiboot compliant image. This image *must* not contain
 * boot modules. Wrap it up in a multiboot adaptor if you want them.
 */
int load_multiboot_image(struct in_addr *server, char *filename,char *cmdline);

/*
 * Return any current character from the console or 0 if none.
 */
int console_trygetchar(void);

/*
 * Debugging support.
 */
#ifndef NDEBUG
#define DPRINTF(fmt, args... ) printf(__FUNCTION__  ":" fmt , ## args)
#else
#define DPRINTF(fmt, args... )
#endif
#define PANIC(fmt, args... ) panic(__FUNCTION__  ":" fmt , ## args)

/*
 * Custom assert that kills the string printf to save space.
 * You will need a version with symbols and GDB to figure it out.
 */
#ifdef	NDEBUG
void	local_panic(void);

#undef  assert
#define assert(expression)  \
	((void)((expression) ? 0 : (local_panic(), 0)))
#endif

#endif /* _BOOT_PXE_DECLS_H_ */
