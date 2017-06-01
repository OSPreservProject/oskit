/*
 * Copyright (c) 2000 University of Utah and the Flux Group.
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

#ifndef _OSKIT_BOOT_BOOTWHAT_H_
#define _OSKIT_BOOT_BOOTWHAT_H_

#define BOOTWHAT_DSTPORT		6969
#define BOOTWHAT_SRCPORT		9696

/*
 * This is the structure we pass back and forth between a oskit kernel
 * and a server running on some other machine, that tells what to do.
 */
#define  MAX_BOOT_DATA		512
#define  MAX_BOOT_PATH		256
#define  MAX_BOOT_CMDLINE	((MAX_BOOT_DATA - MAX_BOOT_PATH) - 32)

typedef struct {
	int	opcode;
	int	status;
	char	data[MAX_BOOT_DATA];
} boot_info_t;

/* Opcode */
#define BIOPCODE_BOOTWHAT_REQUEST	1	/* What to boot request */
#define BIOPCODE_BOOTWHAT_REPLY		2	/* What to boot reply */
#define BIOPCODE_BOOTWHAT_ACK		3	/* Ack to Reply */

/* BOOTWHAT Reply */
typedef struct {
	int	type;
	union {
		/*
		 * Type is BIBOOTWHAT_TYPE_PART
		 *
		 * Specifies the partition number.
		 */
		int			partition;
		
		/*
		 * Type is BIBOOTWHAT_TYPE_SYSID
		 *
		 * Specifies the PC BIOS filesystem type.
		 */
		int			sysid;
		
		/*
		 * Type is BIBOOTWHAT_TYPE_MB
		 *
		 * Specifies a multiboot kernel pathway suitable for TFTP.
		 */
		struct {
			struct in_addr	tftp_ip;
			char		filename[MAX_BOOT_PATH];
		} mb;
	} what;
	char	cmdline[1];
} boot_what_t;

/* What type of thing to boot */
#define BIBOOTWHAT_TYPE_PART	1	/* Boot a partition number */
#define BIBOOTWHAT_TYPE_SYSID	2	/* Boot a system ID */
#define BIBOOTWHAT_TYPE_MB	3	/* Boot a multiboot image */

/* Status */
#define BISTAT_SUCCESS		0
#define BISTAT_FAIL		1

#endif /* _OSKIT_BOOT_BOOTWHAT_H_ */
