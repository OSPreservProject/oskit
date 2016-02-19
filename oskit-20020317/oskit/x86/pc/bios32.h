/*
 * Copyright (c) 1997-1998 University of Utah and the Flux Group.
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
 * Inline functions for common calls to the PC's 16-bit BIOS
 * from 16-bit real or v86 mode.
 */
#ifndef _OSKIT_X86_PC_BIOS32_H_
#define _OSKIT_X86_PC_BIOS32_H_

/* BIOS32 identifying signature: "_32_" */
#define BIOS32_SIGNATURE	(('_'<<0) + ('3'<<8) + ('2'<<16) + ('_'<<24))

/* BIOS32 service identifiers */
#define BIOS32_SERVICE_PCI	(('$'<<0) + ('P'<<8) + ('C'<<16) + ('I'<<24))
#define BIOS32_SERVICE_ACF	(('$'<<0) + ('A'<<8) + ('C'<<16) + ('F'<<24))

/* BIOS32 Service Directory Structure */
struct bios32 {
	unsigned	signature;	/* Must be BIOS32_SIGNATURE */
	unsigned	entry;		/* Physical entrypoint address */
	unsigned char	revision;	/* BIOS32 revision = 0 */
	unsigned char	length;		/* Struct length in paragraphs = 1 */
	unsigned char	checksum;	/* All bytes must sum to zero */
	unsigned char	reserved[5];	/* Must be zero */
};

#endif /* _OSKIT_X86_PC_BIOS32_H_ */
