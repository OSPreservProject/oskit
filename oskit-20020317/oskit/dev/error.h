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
/*
 * Standard error codes returned by device drivers
 * in the OSKit device driver framework.
 *
 * For historical reasons these are currently separated
 * from oskit/error.h, but they will probably be merged.
 */
#ifndef _OSKIT_DEV_ERROR_H_
#define _OSKIT_DEV_ERROR_H_

#include <oskit/error.h>

/*
 * Error codes used by the fdev interfaces.
 * All fdev error codes are really COM result codes.
 * The error codes start at the offset '0x0de0'
 * for easy human identification as fdev error codes.
 */
#define OSKIT_E_DEV_NOSUCH_DEV		0x8f100de1
#define OSKIT_E_DEV_BADOP		0x8f100de2
#define OSKIT_E_DEV_BADPARAM		0x8f100de3
#define OSKIT_E_DEV_IOERR		0x8f100de4
/*      OSKIT_E_OUTOFMEMORY defined in oskit/error.h */
#define OSKIT_E_DEV_NOMORE_CHILDREN	0x8f100de6
#define OSKIT_E_DEV_FIXEDBUS		0x8f100de7
#define OSKIT_E_DEV_NOSUCH_CHILD	0x8f100de8
#define OSKIT_E_DEV_SPACE_INUSE		0x8f100de9
#define OSKIT_E_DEV_SPACE_UNASSIGNED	0x8f100dea
#define OSKIT_E_DEV_BAD_PERIOD		0x8f100deb
#define OSKIT_E_DEV_NO_BIOS32		0x8f100dec
#define OSKIT_E_DEV_NO_BIOS32_SERVICE	0x8f100ded

#endif /* _OSKIT_DEV_ERROR_H_ */
