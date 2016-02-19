/*
 * Copyright (c) 1995-2001 University of Utah and the Flux Group.
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

#include <sys/types.h>
#include <netinet/in.h>

#define FILE_READ_SIZE	1024

int	fileopen(unsigned xport, struct in_addr ip,
		 char *dirname, char *filename);
int	fileread(oskit_addr_t file_ofs, void *buf, oskit_size_t size,
		 oskit_size_t *out_actual);
void	fileclose(void);

#ifdef FILEOPS_NFS
int	nfsopen(struct in_addr ip, char *dirname, char *filename);
int	nfsread(oskit_addr_t file_ofs, void *buf, oskit_size_t size,
		oskit_size_t *out_actual);
void	nfsclose(void);
#else
#define nfsopen		0
#define nfsclose	0
#define nfsread		0
#endif
#ifdef FILEOPS_TFTP
int	tftpopen(struct in_addr ip, char *dirname, char *filename);
int	tftpread(oskit_addr_t file_ofs, void *buf, oskit_size_t size,
		 oskit_size_t *out_actual);
void	tftpclose(void);
#else
#define tftpopen	0
#define tftpclose	0
#define tftpread	0
#endif

