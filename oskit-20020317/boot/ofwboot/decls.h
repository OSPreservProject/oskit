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
#ifndef _BOOT_OFW_DECLS_H_
#define _BOOT_OFW_DECLS_H_

#ifndef BOOT_SERVER
#define BOOT_SERVER	"155.101.128.70"
#endif
#ifndef TFTP_ROOT
#define TFTP_ROOT	"/tftpboot/"
#endif
#ifndef DEF_KERNEL
#define DEF_KERNEL	"/tftpboot/dnard/netbsd/root/netbsd"
#endif

extern oskit_error_t	net_init(void);
extern void		net_shutdown(void);

void			bootinfo_init(void);
int			bootinfo_request(boot_info_t *bootinfo);
void			bootinfo_ack(void);

/*
 * Our DHCP information.
 */
extern char *ipaddr, *netmask, *gateway, *server;

/*
 * Return any current character from the console or 0 if none.
 */
int console_trygetchar(void);

/*
 * Hack delay routine.
 */
void delay(int millis);

#include <assert.h>

#endif /* _BOOT_OFW_DECLS_H_ */
