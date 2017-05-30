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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <malloc.h>

#include "fileops.h"
#include "rpc.h"
#include "debug.h"
#include "socket.h"
#include "misc.h"
#include "perfmon.h"

static struct in_addr ipaddr;			/* IP of server, stashed */
static int nfs_port;
static unsigned char dirfh[NFS_FHSIZE];		/* file handle of dir */
static unsigned char filefh[NFS_FHSIZE];	/* file handle of file */

#ifdef CYCLECOUNTS
long long total_readwait_cycles;
#endif

/*
 * kbytes to readahead, -1 means max
 */
static int kimg_readahead = 32;

/*
 * NFS mount the kernel directory, and perform a lookup on the
 * kernel file.
 *
 * Places the directory filehandle in global "dirfh"
 * Places the kernel filehandle in global filefh
 *
 * returns 0 on success, nonzero on failure
 */
int
fileinit(struct in_addr ip, char *dirname, char *filename)
{
	int			mount_port;
	char			*readahead_option;
	int			err;
	
	readahead_option = getenv("readahead");
	if (readahead_option)
		kimg_readahead = atoi(readahead_option);
	if (kimg_readahead < 0)
		dprintf("NFS using whole-file readahead\n");
	else if (kimg_readahead > 0)
		dprintf("NFS using %dkb readahead\n", kimg_readahead);

	rpc_init();

	/*
	 * Lookup NFS/MOUNTD ports for ROOT using PORTMAP.
	 */
	nfs_port = rpc_lookup(ip, PROG_NFS, 2);
	mount_port = rpc_lookup(ip, PROG_MOUNT, 1);
	if (nfs_port == -1 || mount_port == -1) {
		printf("Can't get root NFS/MOUNT ports.\n");
		return 1;
	}
	dprintf("fileinit: NFS port = %d, MOUNT port = %d\n",
		nfs_port, mount_port);

	/*
	 * NFS mount the directory.
	 */
	dprintf("Mounting %s ... ", dirname);
	err = nfs_mount(ip, mount_port, dirname, dirfh);
	if (err) {
		printf("Can't mount filesystem: %s.\n", nfs_strerror(err));
		return 1;
	}
	dprintf("done\n");

	/*
	 * Lookup the file.
	 */
	dprintf("Looking up %s ... ", filename);
	err = nfs_lookup(ip, nfs_port, dirfh, filename, filefh);
	if (err) {
		printf("Can't lookup %s: %s.\n", filename, nfs_strerror(err));
		return 1;
	}
	dprintf("done\n");

	/* Stash ipaddr for subsequent calls */
	ipaddr.s_addr = ip.s_addr;

	return 0;
}

/*
 * So we can do multiple loads.
 */
void
fileclose(void)
{

}

/*
 * Read a small block of data from the input file.
 */
int
fileread(oskit_addr_t file_ofs, void *buf, oskit_size_t size,
	 oskit_size_t *out_actual)
{
	int n, bytes_read;
	int offset;
	char *bufp = buf;
	int chunk_size;
	int readahead;
#ifdef  CYCLECOUNTS
	unsigned long long before_cycles;
#endif

	bytes_read = 0;
	offset = file_ofs;
	while (size > 0) {
		chunk_size = min(NFS_READ_SIZE, size);
		if (kimg_readahead < 0)
			readahead = (size / NFS_READ_SIZE) - 1;
		else
			readahead = kimg_readahead;

		GETCSTAMP(before_cycles);
		n = nfs_read(ipaddr, nfs_port,
			     filefh, offset, chunk_size, bufp, readahead);
		UPDATECSTAMP(total_readwait_cycles, before_cycles);

		if (n < 0) {
			printf("Unable to read text: %s\n", nfs_strerror(n));
			return 1;	/* XXX Should be an EX_ constant */
		}
		if (n == 0)
			break;		/* hit eof */
		offset += n;
		bufp += n;
		size -= n;
		bytes_read += n;
	}

        *out_actual = bytes_read;
        return 0;
}
