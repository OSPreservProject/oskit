/*
 * Copyright (c) 1996-2001 University of Utah and the Flux Group.
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

#ifndef __DRIVER_H_INCLUDED__
#define __DRIVER_H_INCLUDED__

#include <oskit/error.h>

extern char eth_driver[];
extern char packet[];
extern oskit_size_t packetlen;
extern char *netmask;
extern char *gateway;
extern char *ipaddr;
extern char *domain;
extern char *hostname;
extern int hostnamelen;
extern char **nameservers;

oskit_error_t net_init();
void net_shutdown();

#endif /* __DRIVER_H_INCLUDED__ */
