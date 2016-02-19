/*
 * Copyright (c) 1995-2002 University of Utah and the Flux Group.
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

#ifndef _OSKIT_VERSION_H_
#define _OSKIT_VERSION_H_

/*
 * Define the daily version.  Be sure to change this each release!
 */
#define _OSKIT_VERSION		20020317L
#define _OSKIT_VERSION_STRING  "20020317"

/*
 * Use these in your program if you like to be verbose.
 */
extern unsigned long	oskit_version;
extern char *		oskit_version_string;

/*
 * Or let us do it for you.
 */
extern void oskit_print_version(void);

#endif /* _OSKIT_VERSION_H_ */
